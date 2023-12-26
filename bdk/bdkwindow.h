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

#ifndef __BDK_WINDOW_H__
#define __BDK_WINDOW_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BDK_H_INSIDE__) && !defined (BDK_COMPILATION)
#error "Only <bdk/bdk.h> can be included directly."
#endif

#include <bdk/bdkdrawable.h>
#include <bdk/bdktypes.h>
#include <bdk/bdkevents.h>

G_BEGIN_DECLS

typedef struct _BdkGeometry          BdkGeometry;
typedef struct _BdkWindowAttr        BdkWindowAttr;
typedef struct _BdkPointerHooks      BdkPointerHooks;
typedef struct _BdkWindowRedirect    BdkWindowRedirect;

/* Classes of windows.
 *   InputOutput: Almost every window should be of this type. Such windows
 *		  receive events and are also displayed on screen.
 *   InputOnly: Used only in special circumstances when events need to be
 *		stolen from another window or windows. Input only windows
 *		have no visible output, so they are handy for placing over
 *		top of a group of windows in order to grab the events (or
 *		filter the events) from those windows.
 */
typedef enum
{
  BDK_INPUT_OUTPUT,
  BDK_INPUT_ONLY
} BdkWindowClass;

/* Types of windows.
 *   Root: There is only 1 root window and it is initialized
 *	   at startup. Creating a window of type BDK_WINDOW_ROOT
 *	   is an error.
 *   Toplevel: Windows which interact with the window manager.
 *   Child: Windows which are children of some other type of window.
 *	    (Any other type of window). Most windows are child windows.
 *   Dialog: A special kind of toplevel window which interacts with
 *	     the window manager slightly differently than a regular
 *	     toplevel window. Dialog windows should be used for any
 *	     transient window.
 *   Foreign: A window that actually belongs to another application
 */
typedef enum
{
  BDK_WINDOW_ROOT,
  BDK_WINDOW_TOPLEVEL,
  BDK_WINDOW_CHILD,
  BDK_WINDOW_DIALOG,
  BDK_WINDOW_TEMP,
  BDK_WINDOW_FOREIGN,
  BDK_WINDOW_OFFSCREEN
} BdkWindowType;

/* Window attribute mask values.
 *   BDK_WA_TITLE: The "title" field is valid.
 *   BDK_WA_X: The "x" field is valid.
 *   BDK_WA_Y: The "y" field is valid.
 *   BDK_WA_CURSOR: The "cursor" field is valid.
 *   BDK_WA_COLORMAP: The "colormap" field is valid.
 *   BDK_WA_VISUAL: The "visual" field is valid.
 */
typedef enum
{
  BDK_WA_TITLE	   = 1 << 1,
  BDK_WA_X	   = 1 << 2,
  BDK_WA_Y	   = 1 << 3,
  BDK_WA_CURSOR	   = 1 << 4,
  BDK_WA_COLORMAP  = 1 << 5,
  BDK_WA_VISUAL	   = 1 << 6,
  BDK_WA_WMCLASS   = 1 << 7,
  BDK_WA_NOREDIR   = 1 << 8,
  BDK_WA_TYPE_HINT = 1 << 9
} BdkWindowAttributesType;

/* Size restriction enumeration.
 */
typedef enum
{
  BDK_HINT_POS	       = 1 << 0,
  BDK_HINT_MIN_SIZE    = 1 << 1,
  BDK_HINT_MAX_SIZE    = 1 << 2,
  BDK_HINT_BASE_SIZE   = 1 << 3,
  BDK_HINT_ASPECT      = 1 << 4,
  BDK_HINT_RESIZE_INC  = 1 << 5,
  BDK_HINT_WIN_GRAVITY = 1 << 6,
  BDK_HINT_USER_POS    = 1 << 7,
  BDK_HINT_USER_SIZE   = 1 << 8
} BdkWindowHints;


/* Window type hints.
 * These are hints for the window manager that indicate
 * what type of function the window has. The window manager
 * can use this when determining decoration and behaviour
 * of the window. The hint must be set before mapping the
 * window.
 *
 *   Normal: Normal toplevel window
 *   Dialog: Dialog window
 *   Menu: Window used to implement a menu.
 *   Toolbar: Window used to implement toolbars.
 */
typedef enum
{
  BDK_WINDOW_TYPE_HINT_NORMAL,
  BDK_WINDOW_TYPE_HINT_DIALOG,
  BDK_WINDOW_TYPE_HINT_MENU,		/* Torn off menu */
  BDK_WINDOW_TYPE_HINT_TOOLBAR,
  BDK_WINDOW_TYPE_HINT_SPLASHSCREEN,
  BDK_WINDOW_TYPE_HINT_UTILITY,
  BDK_WINDOW_TYPE_HINT_DOCK,
  BDK_WINDOW_TYPE_HINT_DESKTOP,
  BDK_WINDOW_TYPE_HINT_DROPDOWN_MENU,	/* A drop down menu (from a menubar) */
  BDK_WINDOW_TYPE_HINT_POPUP_MENU,	/* A popup menu (from right-click) */
  BDK_WINDOW_TYPE_HINT_TOOLTIP,
  BDK_WINDOW_TYPE_HINT_NOTIFICATION,
  BDK_WINDOW_TYPE_HINT_COMBO,
  BDK_WINDOW_TYPE_HINT_DND
} BdkWindowTypeHint;

/* The next two enumeration values current match the
 * Motif constants. If this is changed, the implementation
 * of bdk_window_set_decorations/bdk_window_set_functions
 * will need to change as well.
 */
typedef enum
{
  BDK_DECOR_ALL		= 1 << 0,
  BDK_DECOR_BORDER	= 1 << 1,
  BDK_DECOR_RESIZEH	= 1 << 2,
  BDK_DECOR_TITLE	= 1 << 3,
  BDK_DECOR_MENU	= 1 << 4,
  BDK_DECOR_MINIMIZE	= 1 << 5,
  BDK_DECOR_MAXIMIZE	= 1 << 6
} BdkWMDecoration;

typedef enum
{
  BDK_FUNC_ALL		= 1 << 0,
  BDK_FUNC_RESIZE	= 1 << 1,
  BDK_FUNC_MOVE		= 1 << 2,
  BDK_FUNC_MINIMIZE	= 1 << 3,
  BDK_FUNC_MAXIMIZE	= 1 << 4,
  BDK_FUNC_CLOSE	= 1 << 5
} BdkWMFunction;

/* Currently, these are the same values numerically as in the
 * X protocol. If you change that, bdkwindow-x11.c/bdk_window_set_geometry_hints()
 * will need fixing.
 */
typedef enum
{
  BDK_GRAVITY_NORTH_WEST = 1,
  BDK_GRAVITY_NORTH,
  BDK_GRAVITY_NORTH_EAST,
  BDK_GRAVITY_WEST,
  BDK_GRAVITY_CENTER,
  BDK_GRAVITY_EAST,
  BDK_GRAVITY_SOUTH_WEST,
  BDK_GRAVITY_SOUTH,
  BDK_GRAVITY_SOUTH_EAST,
  BDK_GRAVITY_STATIC
} BdkGravity;


typedef enum
{
  BDK_WINDOW_EDGE_NORTH_WEST,
  BDK_WINDOW_EDGE_NORTH,
  BDK_WINDOW_EDGE_NORTH_EAST,
  BDK_WINDOW_EDGE_WEST,
  BDK_WINDOW_EDGE_EAST,
  BDK_WINDOW_EDGE_SOUTH_WEST,
  BDK_WINDOW_EDGE_SOUTH,
  BDK_WINDOW_EDGE_SOUTH_EAST  
} BdkWindowEdge;

struct _BdkWindowAttr
{
  gchar *title;
  gint event_mask;
  gint x, y;
  gint width;
  gint height;
  BdkWindowClass wclass;
  BdkVisual *visual;
  BdkColormap *colormap;
  BdkWindowType window_type;
  BdkCursor *cursor;
  gchar *wmclass_name;
  gchar *wmclass_class;
  gboolean override_redirect;
  BdkWindowTypeHint type_hint;
};

struct _BdkGeometry
{
  gint min_width;
  gint min_height;
  gint max_width;
  gint max_height;
  gint base_width;
  gint base_height;
  gint width_inc;
  gint height_inc;
  gdouble min_aspect;
  gdouble max_aspect;
  BdkGravity win_gravity;
};

struct _BdkPointerHooks 
{
  BdkWindow* (*get_pointer)       (BdkWindow	   *window,
			           gint	           *x,
			           gint   	   *y,
			           BdkModifierType *mask);
  BdkWindow* (*window_at_pointer) (BdkScreen       *screen, /* unused */
                                   gint            *win_x,
                                   gint            *win_y);
};

typedef struct _BdkWindowObject BdkWindowObject;
typedef struct _BdkWindowObjectClass BdkWindowObjectClass;

#define BDK_TYPE_WINDOW              (bdk_window_object_get_type ())
#define BDK_WINDOW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_WINDOW, BdkWindow))
#define BDK_WINDOW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_WINDOW, BdkWindowObjectClass))
#define BDK_IS_WINDOW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_WINDOW))
#define BDK_IS_WINDOW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_WINDOW))
#define BDK_WINDOW_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_WINDOW, BdkWindowObjectClass))

#ifndef BDK_DISABLE_DEPRECATED
#define BDK_WINDOW_OBJECT(object)    ((BdkWindowObject *) BDK_WINDOW (object))

#ifndef BDK_COMPILATION

/* We used to export all of BdkWindowObject, but we don't want to keep doing so.
   However, there are various parts of it accessed by macros and other code,
   so we keep the old exported version public, but in reality it is larger. */

/**** DON'T CHANGE THIS STRUCT, the real version is in bdkinternals.h ****/
struct _BdkWindowObject
{
  BdkDrawable parent_instance;

  BdkDrawable *GSEAL (impl); /* window-system-specific delegate object */
  
  BdkWindowObject *GSEAL (parent);

  gpointer GSEAL (user_data);

  gint GSEAL (x);
  gint GSEAL (y);
  
  gint GSEAL (extension_events);

  GList *GSEAL (filters);
  GList *GSEAL (children);

  BdkColor GSEAL (bg_color);
  BdkPixmap *GSEAL (bg_pixmap);
  
  GSList *GSEAL (paint_stack);
  
  BdkRebunnyion *GSEAL (update_area);
  guint GSEAL (update_freeze_count);
  
  guint8 GSEAL (window_type);
  guint8 GSEAL (depth);
  guint8 GSEAL (resize_count);

  BdkWindowState GSEAL (state);
  
  guint GSEAL (guffaw_gravity) : 1;
  guint GSEAL (input_only) : 1;
  guint GSEAL (modal_hint) : 1;
  guint GSEAL (composited) : 1;
  
  guint GSEAL (destroyed) : 2;

  guint GSEAL (accept_focus) : 1;
  guint GSEAL (focus_on_map) : 1;
  guint GSEAL (shaped) : 1;
  
  BdkEventMask GSEAL (event_mask);

  guint GSEAL (update_and_descendants_freeze_count);

  BdkWindowRedirect *GSEAL (redirect);
};
#endif
#endif

struct _BdkWindowObjectClass
{
  BdkDrawableClass parent_class;
};

/* Windows
 */
GType         bdk_window_object_get_type       (void) G_GNUC_CONST;
BdkWindow*    bdk_window_new                   (BdkWindow     *parent,
                                                BdkWindowAttr *attributes,
                                                gint           attributes_mask);
void          bdk_window_destroy               (BdkWindow     *window);
BdkWindowType bdk_window_get_window_type       (BdkWindow     *window);
gboolean      bdk_window_is_destroyed          (BdkWindow     *window);

BdkScreen*    bdk_window_get_screen            (BdkWindow     *window);
BdkDisplay*   bdk_window_get_display           (BdkWindow     *window);
BdkVisual*    bdk_window_get_visual            (BdkWindow     *window);
int           bdk_window_get_width             (BdkWindow     *window);
int           bdk_window_get_height            (BdkWindow     *window);

BdkWindow*    bdk_window_at_pointer            (gint          *win_x,
                                                gint          *win_y);
void          bdk_window_show                  (BdkWindow     *window);
void          bdk_window_hide                  (BdkWindow     *window);
void          bdk_window_withdraw              (BdkWindow     *window);
void          bdk_window_show_unraised         (BdkWindow     *window);
void          bdk_window_move                  (BdkWindow     *window,
                                                gint           x,
                                                gint           y);
void          bdk_window_resize                (BdkWindow     *window,
                                                gint           width,
                                                gint           height);
void          bdk_window_move_resize           (BdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);
void          bdk_window_reparent              (BdkWindow     *window,
                                                BdkWindow     *new_parent,
                                                gint           x,
                                                gint           y);
void          bdk_window_clear                 (BdkWindow     *window);
void          bdk_window_clear_area            (BdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);
void          bdk_window_clear_area_e          (BdkWindow     *window,
                                                gint           x,
                                                gint           y,
                                                gint           width,
                                                gint           height);
void          bdk_window_raise                 (BdkWindow     *window);
void          bdk_window_lower                 (BdkWindow     *window);
void          bdk_window_restack               (BdkWindow     *window,
						BdkWindow     *sibling,
						gboolean       above);
void          bdk_window_focus                 (BdkWindow     *window,
                                                guint32        timestamp);
void          bdk_window_set_user_data         (BdkWindow     *window,
                                                gpointer       user_data);
void          bdk_window_set_override_redirect (BdkWindow     *window,
                                                gboolean       override_redirect);
gboolean      bdk_window_get_accept_focus      (BdkWindow     *window);
void          bdk_window_set_accept_focus      (BdkWindow     *window,
					        gboolean       accept_focus);
gboolean      bdk_window_get_focus_on_map      (BdkWindow     *window);
void          bdk_window_set_focus_on_map      (BdkWindow     *window,
					        gboolean       focus_on_map);
void          bdk_window_add_filter            (BdkWindow     *window,
                                                BdkFilterFunc  function,
                                                gpointer       data);
void          bdk_window_remove_filter         (BdkWindow     *window,
                                                BdkFilterFunc  function,
                                                gpointer       data);
void          bdk_window_scroll                (BdkWindow     *window,
                                                gint           dx,
                                                gint           dy);
void	      bdk_window_move_rebunnyion           (BdkWindow       *window,
						const BdkRebunnyion *rebunnyion,
						gint             dx,
						gint             dy);
gboolean      bdk_window_ensure_native        (BdkWindow       *window);

/* 
 * This allows for making shaped (partially transparent) windows
 * - cool feature, needed for Drag and Drag for example.
 *  The shape_mask can be the mask
 *  from bdk_pixmap_create_from_xpm.   Stefan Wille
 */
void bdk_window_shape_combine_mask  (BdkWindow	      *window,
                                     BdkBitmap	      *mask,
                                     gint	       x,
                                     gint	       y);
void bdk_window_shape_combine_rebunnyion (BdkWindow	      *window,
                                      const BdkRebunnyion *shape_rebunnyion,
                                      gint	       offset_x,
                                      gint	       offset_y);

/*
 * This routine allows you to quickly take the shapes of all the child windows
 * of a window and use their shapes as the shape mask for this window - useful
 * for container windows that dont want to look like a big box
 * 
 * - Raster
 */
void bdk_window_set_child_shapes (BdkWindow *window);

gboolean bdk_window_get_composited (BdkWindow *window);
void bdk_window_set_composited   (BdkWindow *window,
                                  gboolean   composited);

/*
 * This routine allows you to merge (ie ADD) child shapes to your
 * own window's shape keeping its current shape and ADDING the child
 * shapes to it.
 * 
 * - Raster
 */
void bdk_window_merge_child_shapes         (BdkWindow       *window);

void bdk_window_input_shape_combine_mask   (BdkWindow       *window,
					    BdkBitmap       *mask,
					    gint             x,
					    gint             y);
void bdk_window_input_shape_combine_rebunnyion (BdkWindow       *window,
                                            const BdkRebunnyion *shape_rebunnyion,
                                            gint             offset_x,
                                            gint             offset_y);
void bdk_window_set_child_input_shapes     (BdkWindow       *window);
void bdk_window_merge_child_input_shapes   (BdkWindow       *window);


/*
 * Check if a window has been shown, and whether all its
 * parents up to a toplevel have been shown, respectively.
 * Note that a window that is_viewable below is not necessarily
 * viewable in the X sense.
 */
gboolean bdk_window_is_visible     (BdkWindow *window);
gboolean bdk_window_is_viewable    (BdkWindow *window);
gboolean bdk_window_is_input_only  (BdkWindow *window);
gboolean bdk_window_is_shaped      (BdkWindow *window);

BdkWindowState bdk_window_get_state (BdkWindow *window);

/* Set static bit gravity on the parent, and static
 * window gravity on all children.
 */
gboolean bdk_window_set_static_gravities (BdkWindow *window,
					  gboolean   use_static);   

/* Functions to create/lookup windows from their native equivalents */ 
#if !defined(BDK_DISABLE_DEPRECATED) || defined(BDK_COMPILATION)
#ifndef BDK_MULTIHEAD_SAFE
BdkWindow*    bdk_window_foreign_new (BdkNativeWindow anid);
BdkWindow*    bdk_window_lookup      (BdkNativeWindow anid);
#endif
BdkWindow    *bdk_window_foreign_new_for_display (BdkDisplay      *display,
						  BdkNativeWindow  anid);
BdkWindow*    bdk_window_lookup_for_display (BdkDisplay      *display,
					     BdkNativeWindow  anid);
#endif


/* BdkWindow */

gboolean      bdk_window_has_native      (BdkWindow       *window);
#ifndef BDK_DISABLE_DEPRECATED
void	      bdk_window_set_hints	 (BdkWindow	  *window,
					  gint		   x,
					  gint		   y,
					  gint		   min_width,
					  gint		   min_height,
					  gint		   max_width,
					  gint		   max_height,
					  gint		   flags);
#endif
void              bdk_window_set_type_hint (BdkWindow        *window,
                                            BdkWindowTypeHint hint);
BdkWindowTypeHint bdk_window_get_type_hint (BdkWindow        *window);

gboolean      bdk_window_get_modal_hint   (BdkWindow       *window);
void          bdk_window_set_modal_hint   (BdkWindow       *window,
                                           gboolean         modal);

void bdk_window_set_skip_taskbar_hint (BdkWindow *window,
                                       gboolean   skips_taskbar);
void bdk_window_set_skip_pager_hint   (BdkWindow *window,
                                       gboolean   skips_pager);
void bdk_window_set_urgency_hint      (BdkWindow *window,
				       gboolean   urgent);

void          bdk_window_set_geometry_hints (BdkWindow          *window,
					     const BdkGeometry  *geometry,
					     BdkWindowHints      geom_mask);
#if !defined(BDK_DISABLE_DEPRECATED) || defined(BDK_COMPILATION)
void          bdk_set_sm_client_id          (const gchar        *sm_client_id);
#endif

void	      bdk_window_begin_paint_rect   (BdkWindow          *window,
					     const BdkRectangle *rectangle);
void	      bdk_window_begin_paint_rebunnyion (BdkWindow          *window,
					     const BdkRebunnyion    *rebunnyion);
void	      bdk_window_end_paint          (BdkWindow          *window);
void	      bdk_window_flush             (BdkWindow          *window);

void	      bdk_window_set_title	   (BdkWindow	  *window,
					    const gchar	  *title);
void          bdk_window_set_role          (BdkWindow     *window,
					    const gchar   *role);
void          bdk_window_set_startup_id    (BdkWindow     *window,
					    const gchar   *startup_id);
void          bdk_window_set_transient_for (BdkWindow     *window,
					    BdkWindow     *parent);
void	      bdk_window_set_background	 (BdkWindow	  *window,
					  const BdkColor  *color);
void	      bdk_window_set_back_pixmap (BdkWindow	  *window,
					  BdkPixmap	  *pixmap,
					  gboolean	   parent_relative);
bairo_pattern_t *bdk_window_get_background_pattern (BdkWindow     *window);

void	      bdk_window_set_cursor	 (BdkWindow	  *window,
					  BdkCursor	  *cursor);
BdkCursor    *bdk_window_get_cursor      (BdkWindow       *window);
void	      bdk_window_get_user_data	 (BdkWindow	  *window,
					  gpointer	  *data);
void	      bdk_window_get_geometry	 (BdkWindow	  *window,
					  gint		  *x,
					  gint		  *y,
					  gint		  *width,
					  gint		  *height,
					  gint		  *depth);
void	      bdk_window_get_position	 (BdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
gint	      bdk_window_get_origin	 (BdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
void	      bdk_window_get_root_coords (BdkWindow	  *window,
					  gint             x,
					  gint             y,
					  gint		  *root_x,
					  gint		  *root_y);
void       bdk_window_coords_to_parent   (BdkWindow       *window,
                                          gdouble          x,
                                          gdouble          y,
                                          gdouble         *parent_x,
                                          gdouble         *parent_y);
void       bdk_window_coords_from_parent (BdkWindow       *window,
                                          gdouble          parent_x,
                                          gdouble          parent_y,
                                          gdouble         *x,
                                          gdouble         *y);

#if !defined (BDK_DISABLE_DEPRECATED) || defined (BDK_COMPILATION)
/* Used by btk_handle_box_button_changed () */
gboolean      bdk_window_get_deskrelative_origin (BdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
#endif

void	      bdk_window_get_root_origin (BdkWindow	  *window,
					  gint		  *x,
					  gint		  *y);
void          bdk_window_get_frame_extents (BdkWindow     *window,
                                            BdkRectangle  *rect);
BdkWindow*    bdk_window_get_pointer	 (BdkWindow	  *window,
					  gint		  *x,
					  gint		  *y,
					  BdkModifierType *mask);
BdkWindow *   bdk_window_get_parent      (BdkWindow       *window);
BdkWindow *   bdk_window_get_toplevel    (BdkWindow       *window);

BdkWindow *   bdk_window_get_effective_parent   (BdkWindow *window);
BdkWindow *   bdk_window_get_effective_toplevel (BdkWindow *window);

GList *	      bdk_window_get_children	 (BdkWindow	  *window);
GList *       bdk_window_peek_children   (BdkWindow       *window);
BdkEventMask  bdk_window_get_events	 (BdkWindow	  *window);
void	      bdk_window_set_events	 (BdkWindow	  *window,
					  BdkEventMask	   event_mask);

void          bdk_window_set_icon_list   (BdkWindow       *window,
					  GList           *pixbufs);
void	      bdk_window_set_icon	 (BdkWindow	  *window, 
					  BdkWindow	  *icon_window,
					  BdkPixmap	  *pixmap,
					  BdkBitmap	  *mask);
void	      bdk_window_set_icon_name	 (BdkWindow	  *window, 
					  const gchar	  *name);
void	      bdk_window_set_group	 (BdkWindow	  *window, 
					  BdkWindow	  *leader);
BdkWindow*    bdk_window_get_group	 (BdkWindow	  *window);
void	      bdk_window_set_decorations (BdkWindow	  *window,
					  BdkWMDecoration  decorations);
gboolean      bdk_window_get_decorations (BdkWindow       *window,
					  BdkWMDecoration *decorations);
void	      bdk_window_set_functions	 (BdkWindow	  *window,
					  BdkWMFunction	   functions);
#if !defined(BDK_MULTIHEAD_SAFE) && !defined(BDK_DISABLE_DEPRECATED)
GList *       bdk_window_get_toplevels   (void);
#endif

bairo_surface_t *
              bdk_window_create_similar_surface (BdkWindow *window,
                                          bairo_content_t  content,
                                          int              width,
                                          int              height);

void          bdk_window_beep            (BdkWindow       *window);
void          bdk_window_iconify         (BdkWindow       *window);
void          bdk_window_deiconify       (BdkWindow       *window);
void          bdk_window_stick           (BdkWindow       *window);
void          bdk_window_unstick         (BdkWindow       *window);
void          bdk_window_maximize        (BdkWindow       *window);
void          bdk_window_unmaximize      (BdkWindow       *window);
void          bdk_window_fullscreen      (BdkWindow       *window);
void          bdk_window_unfullscreen    (BdkWindow       *window);
void          bdk_window_set_keep_above  (BdkWindow       *window,
                                          gboolean         setting);
void          bdk_window_set_keep_below  (BdkWindow       *window,
                                          gboolean         setting);
void          bdk_window_set_opacity     (BdkWindow       *window,
                                          gdouble          opacity);
void          bdk_window_register_dnd    (BdkWindow       *window);

void bdk_window_begin_resize_drag (BdkWindow     *window,
                                   BdkWindowEdge  edge,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);
void bdk_window_begin_move_drag   (BdkWindow     *window,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);

/* Interface for dirty-rebunnyion queueing */
void       bdk_window_invalidate_rect           (BdkWindow          *window,
					         const BdkRectangle *rect,
					         gboolean            invalidate_children);
void       bdk_window_invalidate_rebunnyion         (BdkWindow          *window,
					         const BdkRebunnyion    *rebunnyion,
					         gboolean            invalidate_children);
void       bdk_window_invalidate_maybe_recurse  (BdkWindow          *window,
						 const BdkRebunnyion    *rebunnyion,
						 gboolean (*child_func) (BdkWindow *, gpointer),
						 gpointer   user_data);
BdkRebunnyion *bdk_window_get_update_area     (BdkWindow    *window);

void       bdk_window_freeze_updates      (BdkWindow    *window);
void       bdk_window_thaw_updates        (BdkWindow    *window);

void       bdk_window_freeze_toplevel_updates_libbtk_only (BdkWindow *window);
void       bdk_window_thaw_toplevel_updates_libbtk_only   (BdkWindow *window);

void       bdk_window_process_all_updates (void);
void       bdk_window_process_updates     (BdkWindow    *window,
					   gboolean      update_children);

/* Enable/disable flicker, so you can tell if your code is inefficient. */
void       bdk_window_set_debug_updates   (gboolean      setting);

void       bdk_window_constrain_size      (BdkGeometry  *geometry,
                                           guint         flags,
                                           gint          width,
                                           gint          height,
                                           gint         *new_width,
                                           gint         *new_height);

void bdk_window_get_internal_paint_info (BdkWindow    *window,
					 BdkDrawable **real_drawable,
					 gint         *x_offset,
					 gint         *y_offset);

void bdk_window_enable_synchronized_configure (BdkWindow *window);
void bdk_window_configure_finished            (BdkWindow *window);

BdkWindow *bdk_get_default_root_window (void);

/* Offscreen redirection */
BdkPixmap *bdk_offscreen_window_get_pixmap     (BdkWindow     *window);
void       bdk_offscreen_window_set_embedder   (BdkWindow     *window,
						BdkWindow     *embedder);
BdkWindow *bdk_offscreen_window_get_embedder   (BdkWindow     *window);
void       bdk_window_geometry_changed         (BdkWindow     *window);

void       bdk_window_redirect_to_drawable   (BdkWindow     *window,
                                              BdkDrawable   *drawable,
                                              gint           src_x,
                                              gint           src_y,
                                              gint           dest_x,
                                              gint           dest_y,
                                              gint           width,
                                              gint           height);
void       bdk_window_remove_redirection     (BdkWindow     *window);

#ifndef BDK_DISABLE_DEPRECATED
#ifndef BDK_MULTIHEAD_SAFE
BdkPointerHooks *bdk_set_pointer_hooks (const BdkPointerHooks *new_hooks);   
#endif /* BDK_MULTIHEAD_SAFE */

#define BDK_ROOT_PARENT()             (bdk_get_default_root_window ())
#define bdk_window_get_size            bdk_drawable_get_size
#define bdk_window_get_type            bdk_window_get_window_type
#define bdk_window_get_colormap        bdk_drawable_get_colormap
#define bdk_window_set_colormap        bdk_drawable_set_colormap
#define bdk_window_ref                 g_object_ref
#define bdk_window_unref               g_object_unref

#define bdk_window_copy_area(drawable,gc,x,y,source_drawable,source_x,source_y,width,height) \
   bdk_draw_pixmap(drawable,gc,source_drawable,source_x,source_y,x,y,width,height)
#endif /* BDK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __BDK_WINDOW_H__ */
