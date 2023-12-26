/*
 * bdkdisplay-x11.h
 * 
 * Copyright 2001 Sun Microsystems Inc. 
 *
 * Erwann Chenede <erwann.chenede@sun.com>
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
 */

#ifndef __BDK_DISPLAY_X11__
#define __BDK_DISPLAY_X11__

#include <X11/X.h>
#include <X11/Xlib.h>
#include <bunnylib.h>
#include <bdk/bdkdisplay.h>
#include <bdk/bdkkeys.h>
#include <bdk/bdkwindow.h>
#include <bdk/bdkinternals.h>
#include <bdk/bdk.h>		/* For bdk_get_program_class() */

G_BEGIN_DECLS

typedef struct _BdkDisplayX11 BdkDisplayX11;
typedef struct _BdkDisplayX11Class BdkDisplayX11Class;

#define BDK_TYPE_DISPLAY_X11              (_bdk_display_x11_get_type())
#define BDK_DISPLAY_X11(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BDK_TYPE_DISPLAY_X11, BdkDisplayX11))
#define BDK_DISPLAY_X11_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BDK_TYPE_DISPLAY_X11, BdkDisplayX11Class))
#define BDK_IS_DISPLAY_X11(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BDK_TYPE_DISPLAY_X11))
#define BDK_IS_DISPLAY_X11_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BDK_TYPE_DISPLAY_X11))
#define BDK_DISPLAY_X11_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BDK_TYPE_DISPLAY_X11, BdkDisplayX11Class))

typedef enum 
{
  BDK_UNKNOWN,
  BDK_NO,
  BDK_YES
} BdkTristate;

struct _BdkDisplayX11
{
  BdkDisplay parent_instance;
  Display *xdisplay;
  BdkScreen *default_screen;
  BdkScreen **screens;

  GSource *event_source;

  gint grab_count;

  /* Keyboard related information */

  gint xkb_event_type;
  gboolean use_xkb;
  
  /* Whether we were able to turn on detectable-autorepeat using
   * XkbSetDetectableAutorepeat. If FALSE, we'll fall back
   * to checking the next event with XPending(). */
  gboolean have_xkb_autorepeat;

  BdkKeymap *keymap;
  guint	    keymap_serial;

  gboolean use_xshm;
  gboolean have_shm_pixmaps;
  BdkTristate have_render;
  gboolean have_xfixes;
  gint xfixes_event_base;

  gboolean have_xcomposite;
  gboolean have_xdamage;
  gint xdamage_event_base;

  gboolean have_randr13;
  gboolean have_randr15;
  gint xrandr_event_base;

  /* If the SECURITY extension is in place, whether this client holds 
   * a trusted authorization and so is allowed to make various requests 
   * (grabs, properties etc.) Otherwise always TRUE. */
  gboolean trusted_client;

  /* drag and drop information */
  BdkDragContext *current_dest_drag;

  /* data needed for MOTIF DnD */

  Window motif_drag_window;
  BdkWindow *motif_drag_bdk_window;
  GList **motif_target_lists;
  gint motif_n_target_lists;

  /* Mapping to/from virtual atoms */

  GHashTable *atom_from_virtual;
  GHashTable *atom_to_virtual;

  /* Session Management leader window see ICCCM */
  Window leader_window;
  BdkWindow *leader_bdk_window;
  gboolean leader_window_title_set;
  
  /* list of filters for client messages */
  GList *client_filters;

  /* List of functions to go from extension event => X window */
  GSList *event_types;
  
  /* X ID hashtable */
  GHashTable *xid_ht;

  /* translation queue */
  GQueue *translate_queue;

  /* Input device */
  /* input BdkDevice list */
  GList *input_devices;

  /* input BdkWindow list */
  GList *input_windows;

  /* Startup notification */
  gchar *startup_notification_id;

  /* Time of most recent user interaction. */
  gulong user_time;

  /* Sets of atoms for DND */
  guint base_dnd_atoms_precached : 1;
  guint xdnd_atoms_precached : 1;
  guint motif_atoms_precached : 1;
  guint use_sync : 1;

  guint have_shapes : 1;
  guint have_input_shapes : 1;
  gint shape_event_base;

  /* Alpha mask picture format */
  XRenderPictFormat *mask_format;

  /* The offscreen window that has the pointer in it (if any) */
  BdkWindow *active_offscreen_window;
};

struct _BdkDisplayX11Class
{
  BdkDisplayClass parent_class;
};

GType      _bdk_display_x11_get_type            (void);
BdkScreen *_bdk_x11_display_screen_for_xrootwin (BdkDisplay *display,
						 Window      xrootwin);

G_END_DECLS

#endif				/* __BDK_DISPLAY_X11__ */
