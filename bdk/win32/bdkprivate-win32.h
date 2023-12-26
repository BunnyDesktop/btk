/* BDK - The GIMP Drawing Kit
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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BDK_PRIVATE_WIN32_H__
#define __BDK_PRIVATE_WIN32_H__

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT WINVER
#endif

#include <bdk/bdkprivate.h>
#include <bdk/win32/bdkwindow-win32.h>
#include <bdk/win32/bdkpixmap-win32.h>
#include <bdk/win32/bdkwin32keys.h>

#include "bdkinternals.h"

#include "config.h"

/* Make up for some minor w32api or MSVC6 header lossage */

#ifndef PS_JOIN_MASK
#define PS_JOIN_MASK (PS_JOIN_BEVEL|PS_JOIN_MITER|PS_JOIN_ROUND)
#endif

#ifndef FS_VIETNAMESE
#define FS_VIETNAMESE 0x100
#endif

#ifndef WM_GETOBJECT
#define WM_GETOBJECT 0x3D
#endif
#ifndef WM_NCXBUTTONDOWN
#define WM_NCXBUTTONDOWN 0xAB
#endif
#ifndef WM_NCXBUTTONUP
#define WM_NCXBUTTONUP 0xAC
#endif
#ifndef WM_NCXBUTTONDBLCLK
#define WM_NCXBUTTONDBLCLK 0xAD
#endif
#ifndef WM_CHANGEUISTATE
#define WM_CHANGEUISTATE 0x127
#endif
#ifndef WM_UPDATEUISTATE
#define WM_UPDATEUISTATE 0x128
#endif
#ifndef WM_QUERYUISTATE
#define WM_QUERYUISTATE 0x129
#endif
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN 0x20B
#endif
#ifndef WM_XBUTTONUP
#define WM_XBUTTONUP 0x20C
#endif
#ifndef WM_XBUTTONDBLCLK
#define WM_XBUTTONDBLCLK 0x20D
#endif
#ifndef WM_NCMOUSEHOVER
#define WM_NCMOUSEHOVER 0x2A0
#endif
#ifndef WM_NCMOUSELEAVE
#define WM_NCMOUSELEAVE 0x2A2
#endif
#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND 0x319
#endif
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x20E
#endif

#ifndef CF_DIBV5
#define CF_DIBV5 17
#endif


/* Define some combinations of BdkDebugFlags */
#define BDK_DEBUG_EVENTS_OR_COLORMAP (BDK_DEBUG_EVENTS|BDK_DEBUG_COLORMAP)
#define BDK_DEBUG_EVENTS_OR_INPUT (BDK_DEBUG_EVENTS|BDK_DEBUG_INPUT)
#define BDK_DEBUG_PIXMAP_OR_COLORMAP (BDK_DEBUG_PIXMAP|BDK_DEBUG_COLORMAP)
#define BDK_DEBUG_MISC_OR_COLORMAP (BDK_DEBUG_MISC|BDK_DEBUG_COLORMAP)
#define BDK_DEBUG_MISC_OR_EVENTS (BDK_DEBUG_MISC|BDK_DEBUG_EVENTS)

#define BDK_TYPE_GC_WIN32              (_bdk_gc_win32_get_type ())
#define BDK_GC_WIN32(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_GC_WIN32, BdkGCWin32))
#define BDK_GC_WIN32_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_GC_WIN32, BdkGCWin32Class))
#define BDK_IS_GC_WIN32(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_GC_WIN32))
#define BDK_IS_GC_WIN32_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_GC_WIN32))
#define BDK_GC_WIN32_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_GC_WIN32, BdkGCWin32Class))

//#define BDK_WINDOW_SCREEN(win)         (_bdk_screen)
BdkScreen *BDK_WINDOW_SCREEN(BObject *win);

#define BDK_WINDOW_IS_WIN32(win)        (BDK_IS_WINDOW_IMPL_WIN32 (((BdkWindowObject *)win)->impl))

typedef struct _BdkColormapPrivateWin32 BdkColormapPrivateWin32;
typedef struct _BdkCursorPrivate        BdkCursorPrivate;
typedef struct _BdkWin32SingleFont      BdkWin32SingleFont;
typedef struct _BdkFontPrivateWin32     BdkFontPrivateWin32;
typedef struct _BdkGCWin32		BdkGCWin32;
typedef struct _BdkGCWin32Class		BdkGCWin32Class;

struct _BdkCursorPrivate
{
  BdkCursor cursor;
  HCURSOR hcursor;
};

struct _BdkWin32SingleFont
{
  HFONT hfont;
  UINT charset;
  UINT codepage;
  FONTSIGNATURE fs;
};

#ifndef BDK_DISABLE_DEPRECATED

struct _BdkFontPrivateWin32
{
  BdkFontPrivate base;
  GSList *fonts;		/* List of BdkWin32SingleFonts */
  GSList *names;
};

#endif /* BDK_DISABLE_DEPRECATED */

struct _BdkVisualClass
{
  BObjectClass parent_class;
};

typedef enum {
  BDK_WIN32_PE_STATIC,
  BDK_WIN32_PE_AVAILABLE,
  BDK_WIN32_PE_INUSE
} BdkWin32PalEntryState;

struct _BdkColormapPrivateWin32
{
  HPALETTE hpal;
  bint current_size;		/* Current size of hpal */
  BdkWin32PalEntryState *use;
  bint private_val;

  GHashTable *hash;
  BdkColorInfo *info;
};

struct _BdkGCWin32
{
  BdkGC parent_instance;

  /* A Windows Device Context (DC) is not equivalent to an X11
   * GC. We can use a DC only in the window for which it was
   * allocated, or (in the case of a memory DC) with the bitmap that
   * has been selected into it. Thus, we have to release and
   * reallocate a DC each time the BdkGC is used to paint into a new
   * window or pixmap. We thus keep all the necessary values in the
   * BdkGCWin32 object.
   */

  HRGN hcliprgn;

  BdkGCValuesMask values_mask;

  BdkFont *font;
  bint rop2;
  BdkSubwindowMode subwindow_mode;
  bint graphics_exposures;
  bint pen_width;
  DWORD pen_style;
  BdkLineStyle line_style;
  BdkCapStyle cap_style;
  BdkJoinStyle join_style;
  DWORD *pen_dashes;		/* use for PS_USERSTYLE or step-by-step rendering */
  bint pen_num_dashes;
  bint pen_dash_offset;
  HBRUSH pen_hbrbg;

  /* Following fields are valid while the GC exists as a Windows DC */
  HDC hdc;
  int saved_dc;

  HPALETTE holdpal;
};

struct _BdkGCWin32Class
{
  BdkGCClass parent_class;
};

GType _bdk_gc_win32_get_type (void);

bulong _bdk_win32_get_next_tick (bulong suggested_tick);

void _bdk_window_init_position     (BdkWindow *window);
void _bdk_window_move_resize_child (BdkWindow *window,
				    bint       x,
				    bint       y,
				    bint       width,
				    bint       height);

/* BdkWindowImpl methods */
void _bdk_win32_window_scroll (BdkWindow *window,
			       bint       dx,
			       bint       dy);
void _bdk_win32_window_move_rebunnyion (BdkWindow       *window,
				    const BdkRebunnyion *rebunnyion,
				    bint             dx,
				    bint             dy);
void _bdk_win32_windowing_window_get_offsets (BdkWindow *window,
					      bint      *x_offset,
					      bint      *y_offset);


void _bdk_win32_selection_init (void);
void _bdk_win32_dnd_exit (void);

void	 bdk_win32_handle_table_insert  (HANDLE   *handle,
					 bpointer data);
void	 bdk_win32_handle_table_remove  (HANDLE handle);

BdkGC    *_bdk_win32_gc_new             (BdkDrawable        *drawable,
					 BdkGCValues        *values,
					 BdkGCValuesMask     values_mask);

BdkImage *_bdk_win32_get_image 		(BdkDrawable *drawable,
					 bint         x,
					 bint         y,
					 bint         width,
					 bint         height);

BdkImage *_bdk_win32_copy_to_image      (BdkDrawable *drawable,
					 BdkImage    *image,
					 bint         src_x,
					 bint         src_y,
					 bint         dest_x,
					 bint         dest_y,
					 bint         width,
					 bint         height);

void      _bdk_win32_blit               (bboolean              use_fg_bg,
					 BdkDrawableImplWin32 *drawable,
					 BdkGC       	       *gc,
					 BdkDrawable   	       *src,
					 bint        	    	xsrc,
					 bint        	    	ysrc,
					 bint        	    	xdest,
					 bint        	    	ydest,
					 bint        	    	width,
					 bint        	    	height);

COLORREF  _bdk_win32_colormap_color     (BdkColormap *colormap,
				         bulong       pixel);

HRGN	  _bdk_win32_bitmap_to_hrgn     (BdkPixmap   *bitmap);

HRGN	  _bdk_win32_bdkrebunnyion_to_hrgn  (const BdkRebunnyion *rebunnyion,
					 bint             x_origin,
					 bint             y_origin);

BdkRebunnyion *_bdk_win32_hrgn_to_rebunnyion    (HRGN hrgn);

void	_bdk_win32_adjust_client_rect   (BdkWindow *window,
					 RECT      *RECT);

void    _bdk_selection_property_delete (BdkWindow *);

void    _bdk_dropfiles_store (bchar *data);

void    _bdk_wchar_text_handle    (BdkFont       *font,
				   const wchar_t *wcstr,
				   int            wclen,
				   void         (*handler)(BdkWin32SingleFont *,
							   const wchar_t *,
							   int,
							   void *),
				   void          *arg);

void       _bdk_push_modal_window   (BdkWindow *window);
void       _bdk_remove_modal_window (BdkWindow *window);
BdkWindow *_bdk_modal_current       (void);
bboolean   _bdk_modal_blocked       (BdkWindow *window);

#ifdef G_ENABLE_DEBUG
bchar *_bdk_win32_color_to_string      (const BdkColor *color);
void   _bdk_win32_print_paletteentries (const PALETTEENTRY *pep,
					const int           nentries);
void   _bdk_win32_print_system_palette (void);
void   _bdk_win32_print_hpalette       (HPALETTE     hpal);
void   _bdk_win32_print_dc             (HDC          hdc);

bchar *_bdk_win32_cap_style_to_string  (BdkCapStyle  cap_style);
bchar *_bdk_win32_fill_style_to_string (BdkFill      fill);
bchar *_bdk_win32_function_to_string   (BdkFunction  function);
bchar *_bdk_win32_join_style_to_string (BdkJoinStyle join_style);
bchar *_bdk_win32_line_style_to_string (BdkLineStyle line_style);
bchar *_bdk_win32_drag_protocol_to_string (BdkDragProtocol protocol);
bchar *_bdk_win32_gcvalues_mask_to_string (BdkGCValuesMask mask);
bchar *_bdk_win32_window_state_to_string (BdkWindowState state);
bchar *_bdk_win32_window_style_to_string (LONG style);
bchar *_bdk_win32_window_exstyle_to_string (LONG style);
bchar *_bdk_win32_window_pos_bits_to_string (UINT flags);
bchar *_bdk_win32_drag_action_to_string (BdkDragAction actions);
bchar *_bdk_win32_drawable_description (BdkDrawable *d);

bchar *_bdk_win32_rop2_to_string       (int          rop2);
bchar *_bdk_win32_lbstyle_to_string    (UINT         brush_style);
bchar *_bdk_win32_pstype_to_string     (DWORD        pen_style);
bchar *_bdk_win32_psstyle_to_string    (DWORD        pen_style);
bchar *_bdk_win32_psendcap_to_string   (DWORD        pen_style);
bchar *_bdk_win32_psjoin_to_string     (DWORD        pen_style);
bchar *_bdk_win32_message_to_string    (UINT         msg);
bchar *_bdk_win32_key_to_string        (LONG         lParam);
bchar *_bdk_win32_cf_to_string         (UINT         format);
bchar *_bdk_win32_data_to_string       (const buchar*data,
					int          nbytes);
bchar *_bdk_win32_rect_to_string       (const RECT  *rect);

bchar *_bdk_win32_bdkrectangle_to_string (const BdkRectangle *rect);
bchar *_bdk_win32_bdkrebunnyion_to_string    (const BdkRebunnyion    *box);

void   _bdk_win32_print_event            (const BdkEvent     *event);

#endif

bchar  *_bdk_win32_last_error_string (void);
void    _bdk_win32_api_failed        (const bchar *where,
				     const bchar *api);
void    _bdk_other_api_failed        (const bchar *where,
				     const bchar *api);

#define WIN32_API_FAILED(api) _bdk_win32_api_failed (B_STRLOC , api)
#define WIN32_GDI_FAILED(api) WIN32_API_FAILED (api)
#define OTHER_API_FAILED(api) _bdk_other_api_failed (B_STRLOC, api)
 
/* These two macros call a GDI or other Win32 API and if the return
 * value is zero or NULL, print a warning message. The majority of GDI
 * calls return zero or NULL on failure. The value of the macros is nonzero
 * if the call succeeded, zero otherwise.
 */

#define GDI_CALL(api, arglist) (api arglist ? 1 : (WIN32_GDI_FAILED (#api), 0))
#define API_CALL(api, arglist) (api arglist ? 1 : (WIN32_API_FAILED (#api), 0))
 
extern LRESULT CALLBACK _bdk_win32_window_procedure (HWND, UINT, WPARAM, LPARAM);

extern BdkWindow        *_bdk_root;

extern BdkDisplay       *_bdk_display;
extern BdkScreen        *_bdk_screen;

extern bint		 _bdk_num_monitors;
typedef struct _BdkWin32Monitor BdkWin32Monitor;
struct _BdkWin32Monitor
{
  bchar *name;
  bint width_mm, height_mm;
  BdkRectangle rect;
};
extern BdkWin32Monitor  *_bdk_monitors;

/* Offsets to add to Windows coordinates (which are relative to the
 * primary monitor's origin, and thus might be negative for monitors
 * to the left and/or above the primary monitor) to get BDK
 * coordinates, which should be non-negative on the whole screen.
 */
extern bint		 _bdk_offset_x, _bdk_offset_y;

extern HDC		 _bdk_display_hdc;
extern HINSTANCE	 _bdk_dll_hinstance;
extern HINSTANCE	 _bdk_app_hmodule;

/* These are thread specific, but BDK/win32 works OK only when invoked
 * from a single thread anyway.
 */
extern HKL		 _bdk_input_locale;
extern bboolean		 _bdk_input_locale_is_ime;
extern UINT		 _bdk_input_codepage;

extern buint		 _bdk_keymap_serial;

/* BdkAtoms: properties, targets and types */
extern BdkAtom		 _bdk_selection;
extern BdkAtom		 _wm_transient_for;
extern BdkAtom		 _targets;
extern BdkAtom		 _delete;
extern BdkAtom		 _save_targets;
extern BdkAtom           _utf8_string;
extern BdkAtom		 _text;
extern BdkAtom		 _compound_text;
extern BdkAtom		 _text_uri_list;
extern BdkAtom		 _text_html;
extern BdkAtom		 _image_png;
extern BdkAtom		 _image_jpeg;
extern BdkAtom		 _image_bmp;
extern BdkAtom		 _image_gif;

/* DND selections */
extern BdkAtom           _local_dnd;
extern BdkAtom		 _bdk_win32_dropfiles;
extern BdkAtom		 _bdk_ole2_dnd;

/* Clipboard formats */
extern UINT		 _cf_png;
extern UINT		 _cf_jfif;
extern UINT		 _cf_gif;
extern UINT		 _cf_url;
extern UINT		 _cf_html_format;
extern UINT		 _cf_text_html;

/* OLE-based DND state */
typedef enum {
  BDK_WIN32_DND_NONE,
  BDK_WIN32_DND_PENDING,
  BDK_WIN32_DND_DROPPED,
  BDK_WIN32_DND_FAILED,
  BDK_WIN32_DND_DRAGGING,
} BdkWin32DndState;

extern BdkWin32DndState  _dnd_target_state;
extern BdkWin32DndState  _dnd_source_state;

void _bdk_win32_dnd_do_dragdrop (void);
void _bdk_win32_ole2_dnd_property_change (BdkAtom       type,
					  bint          format,
					  const buchar *data,
					  bint          nelements);

void  _bdk_win32_begin_modal_call (void);
void  _bdk_win32_end_modal_call (void);


/* Options */
extern bboolean		 _bdk_input_ignore_wintab;
extern bint		 _bdk_max_colors;

#define BDK_WIN32_COLORMAP_DATA(cmap) ((BdkColormapPrivateWin32 *) BDK_COLORMAP (cmap)->windowing_data)

/* TRUE while a modal sizing, moving, or dnd operation is in progress */
extern bboolean		_modal_operation_in_progress;

extern HWND		_modal_move_resize_window;

/* TRUE when we are emptying the clipboard ourselves */
extern bboolean		_ignore_destroy_clipboard;

/* Mapping from registered clipboard format id (native) to
 * corresponding BdkAtom
 */
extern GHashTable	*_format_atom_table;

/* Hold the result of a delayed rendering */
extern HGLOBAL		_delayed_rendering_data;

HGLOBAL _bdk_win32_selection_convert_to_dib (HGLOBAL  hdata,
					     BdkAtom  target);

/* Convert a pixbuf to an HICON (or HCURSOR).  Supports alpha under
 * Windows XP, thresholds alpha otherwise.
 */
HICON _bdk_win32_pixbuf_to_hicon   (BdkPixbuf *pixbuf);
HICON _bdk_win32_pixbuf_to_hcursor (BdkPixbuf *pixbuf,
				    bint       x_hotspot,
				    bint       y_hotspot);
bboolean _bdk_win32_pixbuf_to_hicon_supports_alpha (void);

void _bdk_win32_append_event (BdkEvent *event);
void _bdk_win32_emit_configure_event (BdkWindow *window);
BdkWindow *_bdk_win32_find_window_for_mouse_event (BdkWindow* reported_window,
						   MSG*       msg);

buint32    _bdk_win32_keymap_get_decimal_mark    (BdkWin32Keymap *keymap);
bboolean   _bdk_win32_keymap_has_altgr           (BdkWin32Keymap *keymap);
buint8     _bdk_win32_keymap_get_active_group    (BdkWin32Keymap *keymap);
buint8     _bdk_win32_keymap_get_rshift_scancode (BdkWin32Keymap *keymap);
void       _bdk_win32_keymap_set_active_layout   (BdkWin32Keymap *keymap,
                                                  HKL             hkl);

/* Initialization */
void _bdk_windowing_window_init (BdkScreen *screen);
void _bdk_root_window_size_init (void);
void _bdk_monitor_init(void);
void _bdk_visual_init (void);
void _bdk_dnd_init    (void);
void _bdk_windowing_image_init  (void);
void _bdk_events_init (void);
void _bdk_input_init  (BdkDisplay *display);

#endif /* __BDK_PRIVATE_WIN32_H__ */
