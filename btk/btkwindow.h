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

#ifndef __BTK_WINDOW_H__
#define __BTK_WINDOW_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkaccelgroup.h>
#include <btk/btkbin.h>


G_BEGIN_DECLS

#define BTK_TYPE_WINDOW			(btk_window_get_type ())
#define BTK_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_WINDOW, BtkWindow))
#define BTK_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_WINDOW, BtkWindowClass))
#define BTK_IS_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_WINDOW))
#define BTK_IS_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_WINDOW))
#define BTK_WINDOW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_WINDOW, BtkWindowClass))


typedef struct _BtkWindowClass        BtkWindowClass;
typedef struct _BtkWindowGeometryInfo BtkWindowGeometryInfo;
typedef struct _BtkWindowGroup        BtkWindowGroup;
typedef struct _BtkWindowGroupClass   BtkWindowGroupClass;

struct _BtkWindow
{
  BtkBin bin;

  gchar *GSEAL (title);
  gchar *GSEAL (wmclass_name);
  gchar *GSEAL (wmclass_class);
  gchar *GSEAL (wm_role);

  BtkWidget *GSEAL (focus_widget);
  BtkWidget *GSEAL (default_widget);
  BtkWindow *GSEAL (transient_parent);
  BtkWindowGeometryInfo *GSEAL (geometry_info);
  BdkWindow *GSEAL (frame);
  BtkWindowGroup *GSEAL (group);

  guint16 GSEAL (configure_request_count);
  guint GSEAL (allow_shrink) : 1;
  guint GSEAL (allow_grow) : 1;
  guint GSEAL (configure_notify_received) : 1;
  /* The following flags are initially TRUE (before a window is mapped).
   * They cause us to compute a configure request that involves
   * default-only parameters. Once mapped, we set them to FALSE.
   * Then we set them to TRUE again on unmap (for position)
   * and on unrealize (for size).
   */
  guint GSEAL (need_default_position) : 1;
  guint GSEAL (need_default_size) : 1;
  guint GSEAL (position) : 3;
  guint GSEAL (type) : 4; /* BtkWindowType */ 
  guint GSEAL (has_user_ref_count) : 1;
  guint GSEAL (has_focus) : 1;

  guint GSEAL (modal) : 1;
  guint GSEAL (destroy_with_parent) : 1;
  
  guint GSEAL (has_frame) : 1;

  /* btk_window_iconify() called before realization */
  guint GSEAL (iconify_initially) : 1;
  guint GSEAL (stick_initially) : 1;
  guint GSEAL (maximize_initially) : 1;
  guint GSEAL (decorated) : 1;
  
  guint GSEAL (type_hint) : 3; /* BdkWindowTypeHint if the hint is one of the original eight. If not, then
				* it contains BDK_WINDOW_TYPE_HINT_NORMAL
				*/
  guint GSEAL (gravity) : 5; /* BdkGravity */

  guint GSEAL (is_active) : 1;
  guint GSEAL (has_toplevel_focus) : 1;
  
  guint GSEAL (frame_left);
  guint GSEAL (frame_top);
  guint GSEAL (frame_right);
  guint GSEAL (frame_bottom);

  guint GSEAL (keys_changed_handler);
  
  BdkModifierType GSEAL (mnemonic_modifier);
  BdkScreen      *GSEAL (screen);
};

struct _BtkWindowClass
{
  BtkBinClass parent_class;

  void     (* set_focus)   (BtkWindow *window,
			    BtkWidget *focus);
  gboolean (* frame_event) (BtkWindow *window,
			    BdkEvent  *event);

  /* G_SIGNAL_ACTION signals for keybindings */

  void     (* activate_focus)          (BtkWindow       *window);
  void     (* activate_default)        (BtkWindow       *window);

  /* as of BTK+ 2.12 the "move-focus" signal has been moved to BtkWidget,
   * so this is merley a virtual function now. Overriding it in subclasses
   * continues to work though.
   */
  void     (* move_focus)              (BtkWindow       *window,
                                        BtkDirectionType direction);
  
  void	   (*keys_changed)	       (BtkWindow	*window);
  
  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

#define BTK_TYPE_WINDOW_GROUP             (btk_window_group_get_type ())
#define BTK_WINDOW_GROUP(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_WINDOW_GROUP, BtkWindowGroup))
#define BTK_WINDOW_GROUP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_WINDOW_GROUP, BtkWindowGroupClass))
#define BTK_IS_WINDOW_GROUP(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_WINDOW_GROUP))
#define BTK_IS_WINDOW_GROUP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_WINDOW_GROUP))
#define BTK_WINDOW_GROUP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_WINDOW_GROUP, BtkWindowGroupClass))

struct _BtkWindowGroup
{
  GObject parent_instance;

  GSList *GSEAL (grabs);
};

struct _BtkWindowGroupClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

#ifdef G_OS_WIN32
/* Reserve old names for DLL ABI backward compatibility */
#define btk_window_set_icon_from_file btk_window_set_icon_from_file_utf8
#define btk_window_set_default_icon_from_file btk_window_set_default_icon_from_file_utf8
#endif

GType      btk_window_get_type                 (void) G_GNUC_CONST;
BtkWidget* btk_window_new                      (BtkWindowType        type);
void       btk_window_set_title                (BtkWindow           *window,
						const gchar         *title);
const gchar *btk_window_get_title              (BtkWindow           *window);
void       btk_window_set_wmclass              (BtkWindow           *window,
						const gchar         *wmclass_name,
						const gchar         *wmclass_class);
void       btk_window_set_role                 (BtkWindow           *window,
                                                const gchar         *role);
void       btk_window_set_startup_id           (BtkWindow           *window,
                                                const gchar         *startup_id);
const gchar *btk_window_get_role               (BtkWindow           *window);
void       btk_window_add_accel_group          (BtkWindow           *window,
						BtkAccelGroup	    *accel_group);
void       btk_window_remove_accel_group       (BtkWindow           *window,
						BtkAccelGroup	    *accel_group);
void       btk_window_set_position             (BtkWindow           *window,
						BtkWindowPosition    position);
gboolean   btk_window_activate_focus	       (BtkWindow           *window);
void       btk_window_set_focus                (BtkWindow           *window,
						BtkWidget           *focus);
BtkWidget *btk_window_get_focus                (BtkWindow           *window);
void       btk_window_set_default              (BtkWindow           *window,
						BtkWidget           *default_widget);
BtkWidget *btk_window_get_default_widget       (BtkWindow           *window);
gboolean   btk_window_activate_default	       (BtkWindow           *window);

void       btk_window_set_transient_for        (BtkWindow           *window, 
						BtkWindow           *parent);
BtkWindow *btk_window_get_transient_for        (BtkWindow           *window);
void       btk_window_set_opacity              (BtkWindow           *window, 
						gdouble              opacity);
gdouble    btk_window_get_opacity              (BtkWindow           *window);
void       btk_window_set_type_hint            (BtkWindow           *window, 
						BdkWindowTypeHint    hint);
BdkWindowTypeHint btk_window_get_type_hint     (BtkWindow           *window);
void       btk_window_set_skip_taskbar_hint    (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_skip_taskbar_hint    (BtkWindow           *window);
void       btk_window_set_skip_pager_hint      (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_skip_pager_hint      (BtkWindow           *window);
void       btk_window_set_urgency_hint         (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_urgency_hint         (BtkWindow           *window);
void       btk_window_set_accept_focus         (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_accept_focus         (BtkWindow           *window);
void       btk_window_set_focus_on_map         (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_focus_on_map         (BtkWindow           *window);
void       btk_window_set_destroy_with_parent  (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_destroy_with_parent  (BtkWindow           *window);
void       btk_window_set_mnemonics_visible    (BtkWindow           *window,
                                                gboolean             setting);
gboolean   btk_window_get_mnemonics_visible    (BtkWindow           *window);

void       btk_window_set_resizable            (BtkWindow           *window,
                                                gboolean             resizable);
gboolean   btk_window_get_resizable            (BtkWindow           *window);

void       btk_window_set_gravity              (BtkWindow           *window,
                                                BdkGravity           gravity);
BdkGravity btk_window_get_gravity              (BtkWindow           *window);


void       btk_window_set_geometry_hints       (BtkWindow           *window,
						BtkWidget           *geometry_widget,
						BdkGeometry         *geometry,
						BdkWindowHints       geom_mask);

void	   btk_window_set_screen	       (BtkWindow	    *window,
						BdkScreen	    *screen);
BdkScreen* btk_window_get_screen	       (BtkWindow	    *window);

gboolean   btk_window_is_active                (BtkWindow           *window);
gboolean   btk_window_has_toplevel_focus       (BtkWindow           *window);


#ifndef BTK_DISABLE_DEPRECATED
/* btk_window_set_has_frame () must be called before realizing the window_*/
void       btk_window_set_has_frame            (BtkWindow *window, 
						gboolean   setting);
gboolean   btk_window_get_has_frame            (BtkWindow *window);
void       btk_window_set_frame_dimensions     (BtkWindow *window, 
						gint       left,
						gint       top,
						gint       right,
						gint       bottom);
void       btk_window_get_frame_dimensions     (BtkWindow *window, 
						gint      *left,
						gint      *top,
						gint      *right,
						gint      *bottom);
#endif
void       btk_window_set_decorated            (BtkWindow *window,
                                                gboolean   setting);
gboolean   btk_window_get_decorated            (BtkWindow *window);
void       btk_window_set_deletable            (BtkWindow *window,
                                                gboolean   setting);
gboolean   btk_window_get_deletable            (BtkWindow *window);

void       btk_window_set_icon_list                (BtkWindow  *window,
                                                    GList      *list);
GList*     btk_window_get_icon_list                (BtkWindow  *window);
void       btk_window_set_icon                     (BtkWindow  *window,
                                                    BdkPixbuf  *icon);
void       btk_window_set_icon_name                (BtkWindow   *window,
						    const gchar *name);
gboolean   btk_window_set_icon_from_file           (BtkWindow   *window,
						    const gchar *filename,
						    GError     **err);
BdkPixbuf* btk_window_get_icon                     (BtkWindow  *window);
const gchar *
           btk_window_get_icon_name                (BtkWindow  *window);
void       btk_window_set_default_icon_list        (GList      *list);
GList*     btk_window_get_default_icon_list        (void);
void       btk_window_set_default_icon             (BdkPixbuf  *icon);
void       btk_window_set_default_icon_name        (const gchar *name);
const gchar *
           btk_window_get_default_icon_name        (void);
gboolean   btk_window_set_default_icon_from_file   (const gchar *filename,
						    GError     **err);

void       btk_window_set_auto_startup_notification (gboolean setting);

/* If window is set modal, input will be grabbed when show and released when hide */
void       btk_window_set_modal      (BtkWindow *window,
				      gboolean   modal);
gboolean   btk_window_get_modal      (BtkWindow *window);
GList*     btk_window_list_toplevels (void);

void     btk_window_add_mnemonic          (BtkWindow       *window,
					   guint            keyval,
					   BtkWidget       *target);
void     btk_window_remove_mnemonic       (BtkWindow       *window,
					   guint            keyval,
					   BtkWidget       *target);
gboolean btk_window_mnemonic_activate     (BtkWindow       *window,
					   guint            keyval,
					   BdkModifierType  modifier);
void     btk_window_set_mnemonic_modifier (BtkWindow       *window,
					   BdkModifierType  modifier);
BdkModifierType btk_window_get_mnemonic_modifier (BtkWindow *window);

gboolean btk_window_activate_key          (BtkWindow        *window,
					   BdkEventKey      *event);
gboolean btk_window_propagate_key_event   (BtkWindow        *window,
					   BdkEventKey      *event);

void     btk_window_present            (BtkWindow *window);
void     btk_window_present_with_time  (BtkWindow *window,
				        guint32    timestamp);
void     btk_window_iconify       (BtkWindow *window);
void     btk_window_deiconify     (BtkWindow *window);
void     btk_window_stick         (BtkWindow *window);
void     btk_window_unstick       (BtkWindow *window);
void     btk_window_maximize      (BtkWindow *window);
void     btk_window_unmaximize    (BtkWindow *window);
void     btk_window_fullscreen    (BtkWindow *window);
void     btk_window_unfullscreen  (BtkWindow *window);
void     btk_window_set_keep_above    (BtkWindow *window, gboolean setting);
void     btk_window_set_keep_below    (BtkWindow *window, gboolean setting);

void btk_window_begin_resize_drag (BtkWindow     *window,
                                   BdkWindowEdge  edge,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);
void btk_window_begin_move_drag   (BtkWindow     *window,
                                   gint           button,
                                   gint           root_x,
                                   gint           root_y,
                                   guint32        timestamp);

#ifndef BTK_DISABLE_DEPRECATED
void       btk_window_set_policy               (BtkWindow           *window,
						gint                 allow_shrink,
						gint                 allow_grow,
						gint                 auto_shrink);
#define	btk_window_position			btk_window_set_position
#endif /* BTK_DISABLE_DEPRECATED */

/* Set initial default size of the window (does not constrain user
 * resize operations)
 */
void     btk_window_set_default_size (BtkWindow   *window,
                                      gint         width,
                                      gint         height);
void     btk_window_get_default_size (BtkWindow   *window,
                                      gint        *width,
                                      gint        *height);
void     btk_window_resize           (BtkWindow   *window,
                                      gint         width,
                                      gint         height);
void     btk_window_get_size         (BtkWindow   *window,
                                      gint        *width,
                                      gint        *height);
void     btk_window_move             (BtkWindow   *window,
                                      gint         x,
                                      gint         y);
void     btk_window_get_position     (BtkWindow   *window,
                                      gint        *root_x,
                                      gint        *root_y);
gboolean btk_window_parse_geometry   (BtkWindow   *window,
                                      const gchar *geometry);
BtkWindowGroup *btk_window_get_group (BtkWindow   *window);
gboolean btk_window_has_group        (BtkWindow   *window);

/* Ignore this unless you are writing a GUI builder */
void     btk_window_reshow_with_initial_size (BtkWindow *window);

BtkWindowType btk_window_get_window_type     (BtkWindow     *window);

/* Window groups
 */
GType            btk_window_group_get_type      (void) G_GNUC_CONST;

BtkWindowGroup * btk_window_group_new           (void);
void             btk_window_group_add_window    (BtkWindowGroup     *window_group,
						 BtkWindow          *window);
void             btk_window_group_remove_window (BtkWindowGroup     *window_group,
					         BtkWindow          *window);
GList *          btk_window_group_list_windows  (BtkWindowGroup     *window_group);


/* --- internal functions --- */
void            _btk_window_internal_set_focus (BtkWindow *window,
						BtkWidget *focus);
void            btk_window_remove_embedded_xid (BtkWindow       *window,
						BdkNativeWindow  xid);
void            btk_window_add_embedded_xid    (BtkWindow       *window,
						BdkNativeWindow  xid);
void            _btk_window_reposition         (BtkWindow *window,
						gint       x,
						gint       y);
void            _btk_window_constrain_size     (BtkWindow *window,
						gint       width,
						gint       height,
						gint      *new_width,
						gint      *new_height);
BtkWidget      *btk_window_group_get_current_grab (BtkWindowGroup *window_group);

void            _btk_window_set_has_toplevel_focus (BtkWindow *window,
						    gboolean   has_toplevel_focus);
void            _btk_window_unset_focus_and_default (BtkWindow *window,
						     BtkWidget *widget);

void            _btk_window_set_is_active          (BtkWindow *window,
						    gboolean   is_active);

void            _btk_window_set_is_toplevel        (BtkWindow *window,
						    gboolean   is_toplevel);

typedef void (*BtkWindowKeysForeachFunc) (BtkWindow      *window,
					  guint           keyval,
					  BdkModifierType modifiers,
					  gboolean        is_mnemonic,
					  gpointer        data);

void _btk_window_keys_foreach (BtkWindow               *window,
			       BtkWindowKeysForeachFunc func,
			       gpointer                 func_data);

/* --- internal (BtkAcceleratable) --- */
gboolean	_btk_window_query_nonaccels	(BtkWindow	*window,
						 guint		 accel_key,
						 BdkModifierType accel_mods);

G_END_DECLS

#endif /* __BTK_WINDOW_H__ */
