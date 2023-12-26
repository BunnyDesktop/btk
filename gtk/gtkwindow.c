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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "bdk/bdk.h"
#include "bdk/bdkkeysyms.h"

#include "btkintl.h"

#include "btkprivate.h"
#include "btkrc.h"
#include "btkwindow.h"
#include "btkwindow-decorate.h"
#include "btkbindings.h"
#include "btkkeyhash.h"
#include "btkmain.h"
#include "btkmnemonichash.h"
#include "btkmenubar.h"
#include "btkiconfactory.h"
#include "btkicontheme.h"
#include "btkmarshalers.h"
#include "btkplug.h"
#include "btkbuildable.h"
#include "btkalias.h"

#ifdef BDK_WINDOWING_X11
#include "x11/bdkx.h"
#endif

enum {
  SET_FOCUS,
  FRAME_EVENT,
  ACTIVATE_FOCUS,
  ACTIVATE_DEFAULT,
  KEYS_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,

  /* Construct */
  PROP_TYPE,

  /* Normal Props */
  PROP_TITLE,
  PROP_ROLE,
  PROP_ALLOW_SHRINK,
  PROP_ALLOW_GROW,
  PROP_RESIZABLE,
  PROP_MODAL,
  PROP_WIN_POS,
  PROP_DEFAULT_WIDTH,
  PROP_DEFAULT_HEIGHT,
  PROP_DESTROY_WITH_PARENT,
  PROP_ICON,
  PROP_ICON_NAME,
  PROP_SCREEN,
  PROP_TYPE_HINT,
  PROP_SKIP_TASKBAR_HINT,
  PROP_SKIP_PAGER_HINT,
  PROP_URGENCY_HINT,
  PROP_ACCEPT_FOCUS,
  PROP_FOCUS_ON_MAP,
  PROP_DECORATED,
  PROP_DELETABLE,
  PROP_GRAVITY,
  PROP_TRANSIENT_FOR,
  PROP_OPACITY,
  
  /* Readonly properties */
  PROP_IS_ACTIVE,
  PROP_HAS_TOPLEVEL_FOCUS,
  
  /* Writeonly properties */
  PROP_STARTUP_ID,
  
  PROP_MNEMONICS_VISIBLE,

  LAST_ARG
};

typedef struct
{
  GList     *icon_list;
  BdkPixmap *icon_pixmap;
  BdkPixmap *icon_mask;
  gchar     *icon_name;
  guint      realized : 1;
  guint      using_default_icon : 1;
  guint      using_parent_icon : 1;
  guint      using_themed_icon : 1;
} BtkWindowIconInfo;

typedef struct {
  BdkGeometry    geometry; /* Last set of geometry hints we set */
  BdkWindowHints flags;
  BdkRectangle   configure_request;
} BtkWindowLastGeometryInfo;

struct _BtkWindowGeometryInfo
{
  /* Properties that the app has set on the window
   */
  BdkGeometry    geometry;	/* Geometry hints */
  BdkWindowHints mask;
  BtkWidget     *widget;	/* subwidget to which hints apply */
  /* from last btk_window_resize () - if > 0, indicates that
   * we should resize to this size.
   */
  gint           resize_width;  
  gint           resize_height;

  /* From last btk_window_move () prior to mapping -
   * only used if initial_pos_set
   */
  gint           initial_x;
  gint           initial_y;
  
  /* Default size - used only the FIRST time we map a window,
   * only if > 0.
   */
  gint           default_width; 
  gint           default_height;
  /* whether to use initial_x, initial_y */
  guint          initial_pos_set : 1;
  /* CENTER_ALWAYS or other position constraint changed since
   * we sent the last configure request.
   */
  guint          position_constraints_changed : 1;

  /* if true, default_width, height come from btk_window_parse_geometry,
   * and thus should be multiplied by the increments and affect the
   * geometry widget only
   */
  guint          default_is_geometry : 1;
  
  BtkWindowLastGeometryInfo last;
};

#define BTK_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_WINDOW, BtkWindowPrivate))

typedef struct _BtkWindowPrivate BtkWindowPrivate;

struct _BtkWindowPrivate
{
  BtkMnemonicHash *mnemonic_hash;
  
  guint above_initially : 1;
  guint below_initially : 1;
  guint fullscreen_initially : 1;
  guint skips_taskbar : 1;
  guint skips_pager : 1;
  guint urgent : 1;
  guint accept_focus : 1;
  guint focus_on_map : 1;
  guint deletable : 1;
  guint transient_parent_group : 1;

  guint reset_type_hint : 1;
  guint opacity_set : 1;
  guint builder_visible : 1;

  guint mnemonics_visible : 1;
  guint mnemonics_visible_set : 1;

  BdkWindowTypeHint type_hint;

  gdouble opacity;

  gchar *startup_id;
};

static void btk_window_dispose            (GObject           *object);
static void btk_window_destroy            (BtkObject         *object);
static void btk_window_finalize           (GObject           *object);
static void btk_window_show               (BtkWidget         *widget);
static void btk_window_hide               (BtkWidget         *widget);
static void btk_window_map                (BtkWidget         *widget);
static void btk_window_unmap              (BtkWidget         *widget);
static void btk_window_realize            (BtkWidget         *widget);
static void btk_window_unrealize          (BtkWidget         *widget);
static void btk_window_size_request       (BtkWidget         *widget,
					   BtkRequisition    *requisition);
static void btk_window_size_allocate      (BtkWidget         *widget,
					   BtkAllocation     *allocation);
static gint btk_window_event              (BtkWidget *widget,
					   BdkEvent *event);
static gboolean btk_window_map_event      (BtkWidget         *widget,
                                           BdkEventAny       *event);
static gboolean btk_window_frame_event    (BtkWindow *window,
					   BdkEvent *event);
static gint btk_window_configure_event    (BtkWidget         *widget,
					   BdkEventConfigure *event);
static gint btk_window_key_press_event    (BtkWidget         *widget,
					   BdkEventKey       *event);
static gint btk_window_key_release_event  (BtkWidget         *widget,
					   BdkEventKey       *event);
static gint btk_window_enter_notify_event (BtkWidget         *widget,
					   BdkEventCrossing  *event);
static gint btk_window_leave_notify_event (BtkWidget         *widget,
					   BdkEventCrossing  *event);
static gint btk_window_focus_in_event     (BtkWidget         *widget,
					   BdkEventFocus     *event);
static gint btk_window_focus_out_event    (BtkWidget         *widget,
					   BdkEventFocus     *event);
static gint btk_window_client_event	  (BtkWidget	     *widget,
					   BdkEventClient    *event);
static void btk_window_check_resize       (BtkContainer      *container);
static gint btk_window_focus              (BtkWidget        *widget,
				           BtkDirectionType  direction);
static void btk_window_real_set_focus     (BtkWindow         *window,
					   BtkWidget         *focus);

static void btk_window_real_activate_default (BtkWindow         *window);
static void btk_window_real_activate_focus   (BtkWindow         *window);
static void btk_window_move_focus            (BtkWindow         *window,
                                              BtkDirectionType   dir);
static void btk_window_keys_changed          (BtkWindow         *window);
static void btk_window_paint                 (BtkWidget         *widget,
					      BdkRectangle      *area);
static gint btk_window_expose                (BtkWidget         *widget,
					      BdkEventExpose    *event);
static void btk_window_unset_transient_for         (BtkWindow  *window);
static void btk_window_transient_parent_realized   (BtkWidget  *parent,
						    BtkWidget  *window);
static void btk_window_transient_parent_unrealized (BtkWidget  *parent,
						    BtkWidget  *window);

static BdkScreen *btk_window_check_screen (BtkWindow *window);

static BtkWindowGeometryInfo* btk_window_get_geometry_info         (BtkWindow    *window,
                                                                    gboolean      create);

static void     btk_window_move_resize               (BtkWindow    *window);
static gboolean btk_window_compare_hints             (BdkGeometry  *geometry_a,
                                                      guint         flags_a,
                                                      BdkGeometry  *geometry_b,
                                                      guint         flags_b);
static void     btk_window_constrain_size            (BtkWindow    *window,
                                                      BdkGeometry  *geometry,
                                                      guint         flags,
                                                      gint          width,
                                                      gint          height,
                                                      gint         *new_width,
                                                      gint         *new_height);
static void     btk_window_constrain_position        (BtkWindow    *window,
                                                      gint          new_width,
                                                      gint          new_height,
                                                      gint         *x,
                                                      gint         *y);
static void     btk_window_compute_hints             (BtkWindow    *window,
                                                      BdkGeometry  *new_geometry,
                                                      guint        *new_flags);
static void     btk_window_compute_configure_request (BtkWindow    *window,
                                                      BdkRectangle *request,
                                                      BdkGeometry  *geometry,
                                                      guint        *flags);

static void     btk_window_set_default_size_internal (BtkWindow    *window,
                                                      gboolean      change_width,
                                                      gint          width,
                                                      gboolean      change_height,
                                                      gint          height,
						      gboolean      is_geometry);

static void     update_themed_icon                    (BtkIconTheme *theme,
				                       BtkWindow    *window);
static GList   *icon_list_from_theme                  (BtkWidget    *widget,
						       const gchar  *name);
static void     btk_window_realize_icon               (BtkWindow    *window);
static void     btk_window_unrealize_icon             (BtkWindow    *window);

static void        btk_window_notify_keys_changed (BtkWindow   *window);
static BtkKeyHash *btk_window_get_key_hash        (BtkWindow   *window);
static void        btk_window_free_key_hash       (BtkWindow   *window);
static void	   btk_window_on_composited_changed (BdkScreen *screen,
						     BtkWindow *window);

static GSList      *toplevel_list = NULL;
static guint        window_signals[LAST_SIGNAL] = { 0 };
static GList       *default_icon_list = NULL;
static gchar       *default_icon_name = NULL;
static guint        default_icon_serial = 0;
static gboolean     disable_startup_notification = FALSE;
static gboolean     sent_startup_notification = FALSE;

static GQuark       quark_btk_embedded = 0;
static GQuark       quark_btk_window_key_hash = 0;
static GQuark       quark_btk_window_default_icon_pixmap = 0;
static GQuark       quark_btk_window_icon_info = 0;
static GQuark       quark_btk_buildable_accels = 0;

static BtkBuildableIface *parent_buildable_iface;

static void btk_window_set_property (GObject         *object,
				     guint            prop_id,
				     const GValue    *value,
				     GParamSpec      *pspec);
static void btk_window_get_property (GObject         *object,
				     guint            prop_id,
				     GValue          *value,
				     GParamSpec      *pspec);

/* BtkBuildable */
static void btk_window_buildable_interface_init  (BtkBuildableIface *iface);
static void btk_window_buildable_set_buildable_property (BtkBuildable        *buildable,
							 BtkBuilder          *builder,
							 const gchar         *name,
							 const GValue        *value);
static void btk_window_buildable_parser_finished (BtkBuildable     *buildable,
						  BtkBuilder       *builder);
static gboolean btk_window_buildable_custom_tag_start (BtkBuildable  *buildable,
						       BtkBuilder    *builder,
						       GObject       *child,
						       const gchar   *tagname,
						       GMarkupParser *parser,
						       gpointer      *data);
static void btk_window_buildable_custom_finished (BtkBuildable  *buildable,
						      BtkBuilder    *builder,
						      GObject       *child,
						      const gchar   *tagname,
						      gpointer       user_data);


G_DEFINE_TYPE_WITH_CODE (BtkWindow, btk_window, BTK_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_window_buildable_interface_init))

static void
add_tab_bindings (BtkBindingSet    *binding_set,
		  BdkModifierType   modifiers,
		  BtkDirectionType  direction)
{
  btk_binding_entry_add_signal (binding_set, BDK_Tab, modifiers,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Tab, modifiers,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
}

static void
add_arrow_bindings (BtkBindingSet    *binding_set,
		    guint             keysym,
		    BtkDirectionType  direction)
{
  guint keypad_keysym = keysym - BDK_Left + BDK_KP_Left;
  
  btk_binding_entry_add_signal (binding_set, keysym, 0,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, keysym, BDK_CONTROL_MASK,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, keypad_keysym, 0,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
  btk_binding_entry_add_signal (binding_set, keypad_keysym, BDK_CONTROL_MASK,
                                "move-focus", 1,
                                BTK_TYPE_DIRECTION_TYPE, direction);
}

static guint32
extract_time_from_startup_id (const gchar* startup_id)
{
  gchar *timestr = g_strrstr (startup_id, "_TIME");
  guint32 retval = BDK_CURRENT_TIME;

  if (timestr)
    {
      gchar *end;
      guint32 timestamp; 
    
      /* Skip past the "_TIME" part */
      timestr += 5;

      errno = 0;
      timestamp = strtoul (timestr, &end, 0);
      if (end != timestr && errno == 0)
        retval = timestamp;
    }

  return retval;
}

static gboolean
startup_id_is_fake (const gchar* startup_id)
{
  return strncmp (startup_id, "_TIME", 5) == 0;
}

static void
btk_window_class_init (BtkWindowClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;
  BtkBindingSet *binding_set;
  
  object_class = (BtkObjectClass*) klass;
  widget_class = (BtkWidgetClass*) klass;
  container_class = (BtkContainerClass*) klass;
  
  quark_btk_embedded = g_quark_from_static_string ("btk-embedded");
  quark_btk_window_key_hash = g_quark_from_static_string ("btk-window-key-hash");
  quark_btk_window_default_icon_pixmap = g_quark_from_static_string ("btk-window-default-icon-pixmap");
  quark_btk_window_icon_info = g_quark_from_static_string ("btk-window-icon-info");
  quark_btk_buildable_accels = g_quark_from_static_string ("btk-window-buildable-accels");

  bobject_class->dispose = btk_window_dispose;
  bobject_class->finalize = btk_window_finalize;

  bobject_class->set_property = btk_window_set_property;
  bobject_class->get_property = btk_window_get_property;
  
  object_class->destroy = btk_window_destroy;

  widget_class->show = btk_window_show;
  widget_class->hide = btk_window_hide;
  widget_class->map = btk_window_map;
  widget_class->map_event = btk_window_map_event;
  widget_class->unmap = btk_window_unmap;
  widget_class->realize = btk_window_realize;
  widget_class->unrealize = btk_window_unrealize;
  widget_class->size_request = btk_window_size_request;
  widget_class->size_allocate = btk_window_size_allocate;
  widget_class->configure_event = btk_window_configure_event;
  widget_class->key_press_event = btk_window_key_press_event;
  widget_class->key_release_event = btk_window_key_release_event;
  widget_class->enter_notify_event = btk_window_enter_notify_event;
  widget_class->leave_notify_event = btk_window_leave_notify_event;
  widget_class->focus_in_event = btk_window_focus_in_event;
  widget_class->focus_out_event = btk_window_focus_out_event;
  widget_class->client_event = btk_window_client_event;
  widget_class->focus = btk_window_focus;
  widget_class->expose_event = btk_window_expose;

  container_class->check_resize = btk_window_check_resize;

  klass->set_focus = btk_window_real_set_focus;
  klass->frame_event = btk_window_frame_event;

  klass->activate_default = btk_window_real_activate_default;
  klass->activate_focus = btk_window_real_activate_focus;
  klass->move_focus = btk_window_move_focus;
  klass->keys_changed = btk_window_keys_changed;
  
  g_type_class_add_private (bobject_class, sizeof (BtkWindowPrivate));
  
  /* Construct */
  g_object_class_install_property (bobject_class,
                                   PROP_TYPE,
                                   g_param_spec_enum ("type",
						      P_("Window Type"),
						      P_("The type of the window"),
						      BTK_TYPE_WINDOW_TYPE,
						      BTK_WINDOW_TOPLEVEL,
						      BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  /* Regular Props */
  g_object_class_install_property (bobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        P_("Window Title"),
                                                        P_("The title of the window"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ROLE,
                                   g_param_spec_string ("role",
							P_("Window Role"),
							P_("Unique identifier for the window to be used when restoring a session"),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkWindow:startup-id:
   *
   * The :startup-id is a write-only property for setting window's
   * startup notification identifier. See btk_window_set_startup_id()
   * for more details.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
                                   PROP_STARTUP_ID,
                                   g_param_spec_string ("startup-id",
							P_("Startup ID"),
							P_("Unique startup identifier for the window used by startup-notification"),
							NULL,
							BTK_PARAM_WRITABLE));

  /**
   * BtkWindow:allow-shrink:
   *
   * If %TRUE, the window has no mimimum size. Setting this to %TRUE is
   * 99&percnt; of the time a bad idea.
   *
   * Deprecated: 2.22: Use BtkWindow:resizable property instead.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ALLOW_SHRINK,
                                   g_param_spec_boolean ("allow-shrink",
							 P_("Allow Shrink"),
							 /* xgettext:no-c-format */
							 P_("If TRUE, the window has no mimimum size. Setting this to TRUE is 99% of the time a bad idea"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  /**
   * BtkWindow:allow-grow:
   *
   * If %TRUE, users can expand the window beyond its minimum size.
   *
   * Deprecated: 2.22: Use BtkWindow:resizable property instead.
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ALLOW_GROW,
                                   g_param_spec_boolean ("allow-grow",
							 P_("Allow Grow"),
							 P_("If TRUE, users can expand the window beyond its minimum size"),
							 TRUE,
							 BTK_PARAM_READWRITE | G_PARAM_DEPRECATED));

  g_object_class_install_property (bobject_class,
                                   PROP_RESIZABLE,
                                   g_param_spec_boolean ("resizable",
							 P_("Resizable"),
							 P_("If TRUE, users can resize the window"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_MODAL,
                                   g_param_spec_boolean ("modal",
							 P_("Modal"),
							 P_("If TRUE, the window is modal (other windows are not usable while this one is up)"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_WIN_POS,
                                   g_param_spec_enum ("window-position",
						      P_("Window Position"),
						      P_("The initial position of the window"),
						      BTK_TYPE_WINDOW_POSITION,
						      BTK_WIN_POS_NONE,
						      BTK_PARAM_READWRITE));
 
  g_object_class_install_property (bobject_class,
                                   PROP_DEFAULT_WIDTH,
                                   g_param_spec_int ("default-width",
						     P_("Default Width"),
						     P_("The default width of the window, used when initially showing the window"),
						     -1,
						     G_MAXINT,
						     -1,
						     BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_DEFAULT_HEIGHT,
                                   g_param_spec_int ("default-height",
						     P_("Default Height"),
						     P_("The default height of the window, used when initially showing the window"),
						     -1,
						     G_MAXINT,
						     -1,
						     BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_DESTROY_WITH_PARENT,
                                   g_param_spec_boolean ("destroy-with-parent",
							 P_("Destroy with Parent"),
							 P_("If this window should be destroyed when the parent is destroyed"),
                                                         FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_ICON,
                                   g_param_spec_object ("icon",
                                                        P_("Icon"),
                                                        P_("Icon for this window"),
                                                        BDK_TYPE_PIXBUF,
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_MNEMONICS_VISIBLE,
                                   g_param_spec_boolean ("mnemonics-visible",
                                                         P_("Mnemonics Visible"),
                                                         P_("Whether mnemonics are currently visible in this window"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  
  /**
   * BtkWindow:icon-name:
   *
   * The :icon-name property specifies the name of the themed icon to
   * use as the window icon. See #BtkIconTheme for more details.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        P_("Icon Name"),
                                                        P_("Name of the themed icon for this window"),
							NULL,
                                                        BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
				   PROP_SCREEN,
				   g_param_spec_object ("screen",
 							P_("Screen"),
 							P_("The screen where this window will be displayed"),
							BDK_TYPE_SCREEN,
 							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
                                   PROP_IS_ACTIVE,
                                   g_param_spec_boolean ("is-active",
							 P_("Is Active"),
							 P_("Whether the toplevel is the current active window"),
							 FALSE,
							 BTK_PARAM_READABLE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_HAS_TOPLEVEL_FOCUS,
                                   g_param_spec_boolean ("has-toplevel-focus",
							 P_("Focus in Toplevel"),
							 P_("Whether the input focus is within this BtkWindow"),
							 FALSE,
							 BTK_PARAM_READABLE));

  g_object_class_install_property (bobject_class,
				   PROP_TYPE_HINT,
				   g_param_spec_enum ("type-hint",
                                                      P_("Type hint"),
                                                      P_("Hint to help the desktop environment understand what kind of window this is and how to treat it."),
                                                      BDK_TYPE_WINDOW_TYPE_HINT,
                                                      BDK_WINDOW_TYPE_HINT_NORMAL,
                                                      BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_SKIP_TASKBAR_HINT,
				   g_param_spec_boolean ("skip-taskbar-hint",
                                                         P_("Skip taskbar"),
                                                         P_("TRUE if the window should not be in the task bar."),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_SKIP_PAGER_HINT,
				   g_param_spec_boolean ("skip-pager-hint",
                                                         P_("Skip pager"),
                                                         P_("TRUE if the window should not be in the pager."),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));  

  g_object_class_install_property (bobject_class,
				   PROP_URGENCY_HINT,
				   g_param_spec_boolean ("urgency-hint",
                                                         P_("Urgent"),
                                                         P_("TRUE if the window should be brought to the user's attention."),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));  

  /**
   * BtkWindow:accept-focus:
   *
   * Whether the window should receive the input focus.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
				   PROP_ACCEPT_FOCUS,
				   g_param_spec_boolean ("accept-focus",
                                                         P_("Accept focus"),
                                                         P_("TRUE if the window should receive the input focus."),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));  

  /**
   * BtkWindow:focus-on-map:
   *
   * Whether the window should receive the input focus when mapped.
   *
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
				   PROP_FOCUS_ON_MAP,
				   g_param_spec_boolean ("focus-on-map",
                                                         P_("Focus on map"),
                                                         P_("TRUE if the window should receive the input focus when mapped."),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));  

  /**
   * BtkWindow:decorated:
   *
   * Whether the window should be decorated by the window manager.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_DECORATED,
                                   g_param_spec_boolean ("decorated",
							 P_("Decorated"),
							 P_("Whether the window should be decorated by the window manager"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  /**
   * BtkWindow:deletable:
   *
   * Whether the window frame should have a close button.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_DELETABLE,
                                   g_param_spec_boolean ("deletable",
							 P_("Deletable"),
							 P_("Whether the window frame should have a close button"),
							 TRUE,
							 BTK_PARAM_READWRITE));


  /**
   * BtkWindow:gravity:
   *
   * The window gravity of the window. See btk_window_move() and #BdkGravity for
   * more details about window gravity.
   *
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_GRAVITY,
                                   g_param_spec_enum ("gravity",
						      P_("Gravity"),
						      P_("The window gravity of the window"),
						      BDK_TYPE_GRAVITY,
						      BDK_GRAVITY_NORTH_WEST,
						      BTK_PARAM_READWRITE));


  /**
   * BtkWindow:transient-for:
   *
   * The transient parent of the window. See btk_window_set_transient_for() for
   * more details about transient windows.
   *
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_TRANSIENT_FOR,
				   g_param_spec_object ("transient-for",
							P_("Transient for Window"),
							P_("The transient parent of the dialog"),
							BTK_TYPE_WINDOW,
							BTK_PARAM_READWRITE| G_PARAM_CONSTRUCT));

  /**
   * BtkWindow:opacity:
   *
   * The requested opacity of the window. See btk_window_set_opacity() for
   * more details about window opacity.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
				   PROP_OPACITY,
				   g_param_spec_double ("opacity",
							P_("Opacity for Window"),
							P_("The opacity of the window, from 0 to 1"),
							0.0,
							1.0,
							1.0,
							BTK_PARAM_READWRITE));

  window_signals[SET_FOCUS] =
    g_signal_new (I_("set-focus"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkWindowClass, set_focus),
                  NULL, NULL,
                  _btk_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  BTK_TYPE_WIDGET);
  
  window_signals[FRAME_EVENT] =
    g_signal_new (I_("frame-event"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET(BtkWindowClass, frame_event),
                  _btk_boolean_handled_accumulator, NULL,
                  _btk_marshal_BOOLEAN__BOXED,
                  G_TYPE_BOOLEAN, 1,
                  BDK_TYPE_EVENT);

  /**
   * BtkWindow::activate-focus:
   * @window: the window which received the signal
   *
   * The ::activate-focus signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user activates the currently
   * focused widget of @window.
   */
  window_signals[ACTIVATE_FOCUS] =
    g_signal_new (I_("activate-focus"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkWindowClass, activate_focus),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  /**
   * BtkWindow::activate-default:
   * @window: the window which received the signal
   *
   * The ::activate-default signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user activates the default widget
   * of @window.
   */
  window_signals[ACTIVATE_DEFAULT] =
    g_signal_new (I_("activate-default"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BtkWindowClass, activate_default),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  /**
   * BtkWindow::keys-changed:
   * @window: the window which received the signal
   *
   * The ::keys-changed signal gets emitted when the set of accelerators
   * or mnemonics that are associated with @window changes.
   */
  window_signals[KEYS_CHANGED] =
    g_signal_new (I_("keys-changed"),
                  G_TYPE_FROM_CLASS (bobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BtkWindowClass, keys_changed),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  /*
   * Key bindings
   */

  binding_set = btk_binding_set_by_class (klass);

  btk_binding_entry_add_signal (binding_set, BDK_space, 0,
                                "activate-focus", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Space, 0,
                                "activate-focus", 0);
  
  btk_binding_entry_add_signal (binding_set, BDK_Return, 0,
                                "activate-default", 0);
  btk_binding_entry_add_signal (binding_set, BDK_ISO_Enter, 0,
                                "activate-default", 0);
  btk_binding_entry_add_signal (binding_set, BDK_KP_Enter, 0,
                                "activate-default", 0);

  add_arrow_bindings (binding_set, BDK_Up, BTK_DIR_UP);
  add_arrow_bindings (binding_set, BDK_Down, BTK_DIR_DOWN);
  add_arrow_bindings (binding_set, BDK_Left, BTK_DIR_LEFT);
  add_arrow_bindings (binding_set, BDK_Right, BTK_DIR_RIGHT);

  add_tab_bindings (binding_set, 0, BTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, BDK_CONTROL_MASK, BTK_DIR_TAB_FORWARD);
  add_tab_bindings (binding_set, BDK_SHIFT_MASK, BTK_DIR_TAB_BACKWARD);
  add_tab_bindings (binding_set, BDK_CONTROL_MASK | BDK_SHIFT_MASK, BTK_DIR_TAB_BACKWARD);
}

static void
btk_window_init (BtkWindow *window)
{
  BdkColormap *colormap;
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (window);
  
  btk_widget_set_has_window (BTK_WIDGET (window), TRUE);
  _btk_widget_set_is_toplevel (BTK_WIDGET (window), TRUE);

  BTK_PRIVATE_SET_FLAG (window, BTK_ANCHORED);

  btk_container_set_resize_mode (BTK_CONTAINER (window), BTK_RESIZE_QUEUE);

  window->title = NULL;
  window->wmclass_name = g_strdup (g_get_prgname ());
  window->wmclass_class = g_strdup (bdk_get_program_class ());
  window->wm_role = NULL;
  window->geometry_info = NULL;
  window->type = BTK_WINDOW_TOPLEVEL;
  window->focus_widget = NULL;
  window->default_widget = NULL;
  window->configure_request_count = 0;
  window->allow_shrink = FALSE;
  window->allow_grow = TRUE;
  window->configure_notify_received = FALSE;
  window->position = BTK_WIN_POS_NONE;
  window->need_default_size = TRUE;
  window->need_default_position = TRUE;
  window->modal = FALSE;
  window->frame = NULL;
  window->has_frame = FALSE;
  window->frame_left = 0;
  window->frame_right = 0;
  window->frame_top = 0;
  window->frame_bottom = 0;
  window->type_hint = BDK_WINDOW_TYPE_HINT_NORMAL;
  window->gravity = BDK_GRAVITY_NORTH_WEST;
  window->decorated = TRUE;
  window->mnemonic_modifier = BDK_MOD1_MASK;
  window->screen = bdk_screen_get_default ();

  priv->accept_focus = TRUE;
  priv->focus_on_map = TRUE;
  priv->deletable = TRUE;
  priv->type_hint = BDK_WINDOW_TYPE_HINT_NORMAL;
  priv->opacity = 1.0;
  priv->startup_id = NULL;
  priv->mnemonics_visible = TRUE;

  colormap = _btk_widget_peek_colormap ();
  if (colormap)
    btk_widget_set_colormap (BTK_WIDGET (window), colormap);
  
  g_object_ref_sink (window);
  window->has_user_ref_count = TRUE;
  toplevel_list = g_slist_prepend (toplevel_list, window);

  btk_decorated_window_init (window);

  g_signal_connect (window->screen, "composited-changed",
		    G_CALLBACK (btk_window_on_composited_changed), window);
}

static void
btk_window_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  BtkWindow  *window;
  BtkWindowPrivate *priv;
  
  window = BTK_WINDOW (object);

  priv = BTK_WINDOW_GET_PRIVATE (window);

  switch (prop_id)
    {
    case PROP_TYPE:
      window->type = g_value_get_enum (value);
      break;
    case PROP_TITLE:
      btk_window_set_title (window, g_value_get_string (value));
      break;
    case PROP_ROLE:
      btk_window_set_role (window, g_value_get_string (value));
      break;
    case PROP_STARTUP_ID:
      btk_window_set_startup_id (window, g_value_get_string (value));
      break; 
    case PROP_ALLOW_SHRINK:
      window->allow_shrink = g_value_get_boolean (value);
      btk_widget_queue_resize (BTK_WIDGET (window));
      break;
    case PROP_ALLOW_GROW:
      window->allow_grow = g_value_get_boolean (value);
      btk_widget_queue_resize (BTK_WIDGET (window));
      g_object_notify (G_OBJECT (window), "resizable");
      break;
    case PROP_RESIZABLE:
      window->allow_grow = g_value_get_boolean (value);
      btk_widget_queue_resize (BTK_WIDGET (window));
      g_object_notify (G_OBJECT (window), "allow-grow");
      break;
    case PROP_MODAL:
      btk_window_set_modal (window, g_value_get_boolean (value));
      break;
    case PROP_WIN_POS:
      btk_window_set_position (window, g_value_get_enum (value));
      break;
    case PROP_DEFAULT_WIDTH:
      btk_window_set_default_size_internal (window,
                                            TRUE, g_value_get_int (value),
                                            FALSE, -1, FALSE);
      break;
    case PROP_DEFAULT_HEIGHT:
      btk_window_set_default_size_internal (window,
                                            FALSE, -1,
                                            TRUE, g_value_get_int (value), FALSE);
      break;
    case PROP_DESTROY_WITH_PARENT:
      btk_window_set_destroy_with_parent (window, g_value_get_boolean (value));
      break;
    case PROP_ICON:
      btk_window_set_icon (window,
                           g_value_get_object (value));
      break;
    case PROP_ICON_NAME:
      btk_window_set_icon_name (window, g_value_get_string (value));
      break;
    case PROP_SCREEN:
      btk_window_set_screen (window, g_value_get_object (value));
      break;
    case PROP_TYPE_HINT:
      btk_window_set_type_hint (window,
                                g_value_get_enum (value));
      break;
    case PROP_SKIP_TASKBAR_HINT:
      btk_window_set_skip_taskbar_hint (window,
                                        g_value_get_boolean (value));
      break;
    case PROP_SKIP_PAGER_HINT:
      btk_window_set_skip_pager_hint (window,
                                      g_value_get_boolean (value));
      break;
    case PROP_URGENCY_HINT:
      btk_window_set_urgency_hint (window,
				   g_value_get_boolean (value));
      break;
    case PROP_ACCEPT_FOCUS:
      btk_window_set_accept_focus (window,
				   g_value_get_boolean (value));
      break;
    case PROP_FOCUS_ON_MAP:
      btk_window_set_focus_on_map (window,
				   g_value_get_boolean (value));
      break;
    case PROP_DECORATED:
      btk_window_set_decorated (window, g_value_get_boolean (value));
      break;
    case PROP_DELETABLE:
      btk_window_set_deletable (window, g_value_get_boolean (value));
      break;
    case PROP_GRAVITY:
      btk_window_set_gravity (window, g_value_get_enum (value));
      break;
    case PROP_TRANSIENT_FOR:
      btk_window_set_transient_for (window, g_value_get_object (value));
      break;
    case PROP_OPACITY:
      btk_window_set_opacity (window, g_value_get_double (value));
      break;
    case PROP_MNEMONICS_VISIBLE:
      btk_window_set_mnemonics_visible (window, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_window_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  BtkWindow  *window;
  BtkWindowPrivate *priv;

  window = BTK_WINDOW (object);
  priv = BTK_WINDOW_GET_PRIVATE (window);
  
  switch (prop_id)
    {
      BtkWindowGeometryInfo *info;
    case PROP_TYPE:
      g_value_set_enum (value, window->type);
      break;
    case PROP_ROLE:
      g_value_set_string (value, window->wm_role);
      break;
    case PROP_TITLE:
      g_value_set_string (value, window->title);
      break;
    case PROP_ALLOW_SHRINK:
      g_value_set_boolean (value, window->allow_shrink);
      break;
    case PROP_ALLOW_GROW:
      g_value_set_boolean (value, window->allow_grow);
      break;
    case PROP_RESIZABLE:
      g_value_set_boolean (value, window->allow_grow);
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, window->modal);
      break;
    case PROP_WIN_POS:
      g_value_set_enum (value, window->position);
      break;
    case PROP_DEFAULT_WIDTH:
      info = btk_window_get_geometry_info (window, FALSE);
      if (!info)
	g_value_set_int (value, -1);
      else
	g_value_set_int (value, info->default_width);
      break;
    case PROP_DEFAULT_HEIGHT:
      info = btk_window_get_geometry_info (window, FALSE);
      if (!info)
	g_value_set_int (value, -1);
      else
	g_value_set_int (value, info->default_height);
      break;
    case PROP_DESTROY_WITH_PARENT:
      g_value_set_boolean (value, window->destroy_with_parent);
      break;
    case PROP_ICON:
      g_value_set_object (value, btk_window_get_icon (window));
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, btk_window_get_icon_name (window));
      break;
    case PROP_SCREEN:
      g_value_set_object (value, window->screen);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, window->is_active);
      break;
    case PROP_HAS_TOPLEVEL_FOCUS:
      g_value_set_boolean (value, window->has_toplevel_focus);
      break;
    case PROP_TYPE_HINT:
      g_value_set_enum (value, priv->type_hint);
      break;
    case PROP_SKIP_TASKBAR_HINT:
      g_value_set_boolean (value,
                           btk_window_get_skip_taskbar_hint (window));
      break;
    case PROP_SKIP_PAGER_HINT:
      g_value_set_boolean (value,
                           btk_window_get_skip_pager_hint (window));
      break;
    case PROP_URGENCY_HINT:
      g_value_set_boolean (value,
                           btk_window_get_urgency_hint (window));
      break;
    case PROP_ACCEPT_FOCUS:
      g_value_set_boolean (value,
                           btk_window_get_accept_focus (window));
      break;
    case PROP_FOCUS_ON_MAP:
      g_value_set_boolean (value,
                           btk_window_get_focus_on_map (window));
      break;
    case PROP_DECORATED:
      g_value_set_boolean (value, btk_window_get_decorated (window));
      break;
    case PROP_DELETABLE:
      g_value_set_boolean (value, btk_window_get_deletable (window));
      break;
    case PROP_GRAVITY:
      g_value_set_enum (value, btk_window_get_gravity (window));
      break;
    case PROP_TRANSIENT_FOR:
      g_value_set_object (value, btk_window_get_transient_for (window));
      break;
    case PROP_OPACITY:
      g_value_set_double (value, btk_window_get_opacity (window));
      break;
    case PROP_MNEMONICS_VISIBLE:
      g_value_set_boolean (value, priv->mnemonics_visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_window_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->set_buildable_property = btk_window_buildable_set_buildable_property;
  iface->parser_finished = btk_window_buildable_parser_finished;
  iface->custom_tag_start = btk_window_buildable_custom_tag_start;
  iface->custom_finished = btk_window_buildable_custom_finished;
}

static void
btk_window_buildable_set_buildable_property (BtkBuildable        *buildable,
					     BtkBuilder          *builder,
					     const gchar         *name,
					     const GValue        *value)
{
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (buildable);

  if (strcmp (name, "visible") == 0 && g_value_get_boolean (value))
    priv->builder_visible = TRUE;
  else
    parent_buildable_iface->set_buildable_property (buildable, builder, name, value);
}

static void
btk_window_buildable_parser_finished (BtkBuildable *buildable,
				      BtkBuilder   *builder)
{
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (buildable);
  GObject *object;
  GSList *accels, *l;

  if (priv->builder_visible)
    btk_widget_show (BTK_WIDGET (buildable));

  accels = g_object_get_qdata (G_OBJECT (buildable), quark_btk_buildable_accels);
  for (l = accels; l; l = l->next)
    {
      object = btk_builder_get_object (builder, l->data);
      if (!object)
	{
	  g_warning ("Unknown accel group %s specified in window %s",
		     (const gchar*)l->data, btk_buildable_get_name (buildable));
	  continue;
	}
      btk_window_add_accel_group (BTK_WINDOW (buildable),
				  BTK_ACCEL_GROUP (object));
      g_free (l->data);
    }

  g_object_set_qdata (G_OBJECT (buildable), quark_btk_buildable_accels, NULL);

  parent_buildable_iface->parser_finished (buildable, builder);
}

typedef struct {
  GObject *object;
  GSList *items;
} GSListSubParserData;

static void
window_start_element (GMarkupParseContext *context,
			  const gchar         *element_name,
			  const gchar        **names,
			  const gchar        **values,
			  gpointer            user_data,
			  GError            **error)
{
  guint i;
  GSListSubParserData *data = (GSListSubParserData*)user_data;

  if (strcmp (element_name, "group") == 0)
    {
      for (i = 0; names[i]; i++)
	{
	  if (strcmp (names[i], "name") == 0)
	    data->items = g_slist_prepend (data->items, g_strdup (values[i]));
	}
    }
  else if (strcmp (element_name, "accel-groups") == 0)
    return;
  else
    g_warning ("Unsupported tag type for BtkWindow: %s\n",
	       element_name);

}

static const GMarkupParser window_parser =
  {
    window_start_element
  };

static gboolean
btk_window_buildable_custom_tag_start (BtkBuildable  *buildable,
				       BtkBuilder    *builder,
				       GObject       *child,
				       const gchar   *tagname,
				       GMarkupParser *parser,
				       gpointer      *data)
{
  GSListSubParserData *parser_data;

  if (parent_buildable_iface->custom_tag_start (buildable, builder, child, 
						tagname, parser, data))
    return TRUE;

  if (strcmp (tagname, "accel-groups") == 0)
    {
      parser_data = g_slice_new0 (GSListSubParserData);
      parser_data->items = NULL;
      parser_data->object = G_OBJECT (buildable);

      *parser = window_parser;
      *data = parser_data;
      return TRUE;
    }

  return FALSE;
}

static void
btk_window_buildable_custom_finished (BtkBuildable  *buildable,
					  BtkBuilder    *builder,
					  GObject       *child,
					  const gchar   *tagname,
					  gpointer       user_data)
{
  GSListSubParserData *data;

  parent_buildable_iface->custom_finished (buildable, builder, child, 
					   tagname, user_data);

  if (strcmp (tagname, "accel-groups") != 0)
    return;
  
  data = (GSListSubParserData*)user_data;

  g_object_set_qdata_full (G_OBJECT (buildable), quark_btk_buildable_accels, 
			   data->items, (GDestroyNotify) g_slist_free);

  g_slice_free (GSListSubParserData, data);
}

/**
 * btk_window_new:
 * @type: type of window
 * 
 * Creates a new #BtkWindow, which is a toplevel window that can
 * contain other widgets. Nearly always, the type of the window should
 * be #BTK_WINDOW_TOPLEVEL. If you're implementing something like a
 * popup menu from scratch (which is a bad idea, just use #BtkMenu),
 * you might use #BTK_WINDOW_POPUP. #BTK_WINDOW_POPUP is not for
 * dialogs, though in some other toolkits dialogs are called "popups".
 * In BTK+, #BTK_WINDOW_POPUP means a pop-up menu or pop-up tooltip.
 * On X11, popup windows are not controlled by the <link
 * linkend="btk-X11-arch">window manager</link>.
 *
 * If you simply want an undecorated window (no window borders), use
 * btk_window_set_decorated(), don't use #BTK_WINDOW_POPUP.
 * 
 * Return value: a new #BtkWindow.
 **/
BtkWidget*
btk_window_new (BtkWindowType type)
{
  BtkWindow *window;

  g_return_val_if_fail (type >= BTK_WINDOW_TOPLEVEL && type <= BTK_WINDOW_POPUP, NULL);

  window = g_object_new (BTK_TYPE_WINDOW, NULL);

  window->type = type;

  return BTK_WIDGET (window);
}

/**
 * btk_window_set_title:
 * @window: a #BtkWindow
 * @title: title of the window
 * 
 * Sets the title of the #BtkWindow. The title of a window will be
 * displayed in its title bar; on the X Window System, the title bar
 * is rendered by the <link linkend="btk-X11-arch">window
 * manager</link>, so exactly how the title appears to users may vary
 * according to a user's exact configuration. The title should help a
 * user distinguish this window from other windows they may have
 * open. A good title might include the application name and current
 * document filename, for example.
 * 
 **/
void
btk_window_set_title (BtkWindow   *window,
		      const gchar *title)
{
  char *new_title;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  new_title = g_strdup (title);
  g_free (window->title);
  window->title = new_title;

  if (btk_widget_get_realized (BTK_WIDGET (window)))
    {
      bdk_window_set_title (BTK_WIDGET (window)->window, window->title);

      btk_decorated_window_set_title (window, title);
    }

  g_object_notify (G_OBJECT (window), "title");
}

/**
 * btk_window_get_title:
 * @window: a #BtkWindow
 *
 * Retrieves the title of the window. See btk_window_set_title().
 *
 * Return value: the title of the window, or %NULL if none has
 *    been set explicitely. The returned string is owned by the widget
 *    and must not be modified or freed.
 **/
const gchar *
btk_window_get_title (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  return window->title;
}

/**
 * btk_window_set_wmclass:
 * @window: a #BtkWindow
 * @wmclass_name: window name hint
 * @wmclass_class: window class hint
 *
 * Don't use this function. It sets the X Window System "class" and
 * "name" hints for a window.  According to the ICCCM, you should
 * always set these to the same value for all windows in an
 * application, and BTK+ sets them to that value by default, so calling
 * this function is sort of pointless. However, you may want to call
 * btk_window_set_role() on each window in your application, for the
 * benefit of the session manager. Setting the role allows the window
 * manager to restore window positions when loading a saved session.
 * 
 **/
void
btk_window_set_wmclass (BtkWindow *window,
			const gchar *wmclass_name,
			const gchar *wmclass_class)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  g_free (window->wmclass_name);
  window->wmclass_name = g_strdup (wmclass_name);

  g_free (window->wmclass_class);
  window->wmclass_class = g_strdup (wmclass_class);

  if (btk_widget_get_realized (BTK_WIDGET (window)))
    g_warning ("btk_window_set_wmclass: shouldn't set wmclass after window is realized!\n");
}

/**
 * btk_window_set_role:
 * @window: a #BtkWindow
 * @role: unique identifier for the window to be used when restoring a session
 *
 * This function is only useful on X11, not with other BTK+ targets.
 * 
 * In combination with the window title, the window role allows a
 * <link linkend="btk-X11-arch">window manager</link> to identify "the
 * same" window when an application is restarted. So for example you
 * might set the "toolbox" role on your app's toolbox window, so that
 * when the user restarts their session, the window manager can put
 * the toolbox back in the same place.
 *
 * If a window already has a unique title, you don't need to set the
 * role, since the WM can use the title to identify the window when
 * restoring the session.
 * 
 **/
void
btk_window_set_role (BtkWindow   *window,
                     const gchar *role)
{
  char *new_role;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  new_role = g_strdup (role);
  g_free (window->wm_role);
  window->wm_role = new_role;

  if (btk_widget_get_realized (BTK_WIDGET (window)))
    bdk_window_set_role (BTK_WIDGET (window)->window, window->wm_role);

  g_object_notify (G_OBJECT (window), "role");
}

/**
 * btk_window_set_startup_id:
 * @window: a #BtkWindow
 * @startup_id: a string with startup-notification identifier
 *
 * Startup notification identifiers are used by desktop environment to 
 * track application startup, to provide user feedback and other 
 * features. This function changes the corresponding property on the
 * underlying BdkWindow. Normally, startup identifier is managed 
 * automatically and you should only use this function in special cases
 * like transferring focus from other processes. You should use this
 * function before calling btk_window_present() or any equivalent
 * function generating a window map event.
 *
 * This function is only useful on X11, not with other BTK+ targets.
 * 
 * Since: 2.12
 **/
void
btk_window_set_startup_id (BtkWindow   *window,
                           const gchar *startup_id)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));

  priv = BTK_WINDOW_GET_PRIVATE (window);
  
  g_free (priv->startup_id);
  priv->startup_id = g_strdup (startup_id);

  if (btk_widget_get_realized (BTK_WIDGET (window)))
    {
      guint32 timestamp = extract_time_from_startup_id (priv->startup_id);

#ifdef BDK_WINDOWING_X11
      if (timestamp != BDK_CURRENT_TIME)
	bdk_x11_window_set_user_time (BTK_WIDGET (window)->window, timestamp);
#endif

      /* Here we differentiate real and "fake" startup notification IDs,
       * constructed on purpose just to pass interaction timestamp
       */
      if (startup_id_is_fake (priv->startup_id))
	btk_window_present_with_time (window, timestamp);
      else 
        {
          bdk_window_set_startup_id (BTK_WIDGET (window)->window,
                                     priv->startup_id);
          
          /* If window is mapped, terminate the startup-notification too */
          if (btk_widget_get_mapped (BTK_WIDGET (window)) &&
              !disable_startup_notification)
            bdk_notify_startup_complete_with_id (priv->startup_id);
        }
    }

  g_object_notify (G_OBJECT (window), "startup-id");
}

/**
 * btk_window_get_role:
 * @window: a #BtkWindow
 *
 * Returns the role of the window. See btk_window_set_role() for
 * further explanation.
 *
 * Return value: the role of the window if set, or %NULL. The
 *   returned is owned by the widget and must not be modified
 *   or freed.
 **/
const gchar *
btk_window_get_role (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  return window->wm_role;
}

/**
 * btk_window_set_focus:
 * @window: a #BtkWindow
 * @focus: (allow-none): widget to be the new focus widget, or %NULL to unset
 *   any focus widget for the toplevel window.
 *
 * If @focus is not the current focus widget, and is focusable, sets
 * it as the focus widget for the window. If @focus is %NULL, unsets
 * the focus widget for this window. To set the focus to a particular
 * widget in the toplevel, it is usually more convenient to use
 * btk_widget_grab_focus() instead of this function.
 **/
void
btk_window_set_focus (BtkWindow *window,
		      BtkWidget *focus)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  if (focus)
    {
      g_return_if_fail (BTK_IS_WIDGET (focus));
      g_return_if_fail (btk_widget_get_can_focus (focus));
    }

  if (focus)
    btk_widget_grab_focus (focus);
  else
    {
      /* Clear the existing focus chain, so that when we focus into
       * the window again, we start at the beginnning.
       */
      BtkWidget *widget = window->focus_widget;
      if (widget)
	{
	  while (widget->parent)
	    {
	      widget = widget->parent;
	      btk_container_set_focus_child (BTK_CONTAINER (widget), NULL);
	    }
	}
      
      _btk_window_internal_set_focus (window, NULL);
    }
}

void
_btk_window_internal_set_focus (BtkWindow *window,
				BtkWidget *focus)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  if ((window->focus_widget != focus) ||
      (focus && !btk_widget_has_focus (focus)))
    g_signal_emit (window, window_signals[SET_FOCUS], 0, focus);
}

/**
 * btk_window_set_default:
 * @window: a #BtkWindow
 * @default_widget: (allow-none): widget to be the default, or %NULL to unset the
 *                  default widget for the toplevel.
 *
 * The default widget is the widget that's activated when the user
 * presses Enter in a dialog (for example). This function sets or
 * unsets the default widget for a #BtkWindow about. When setting
 * (rather than unsetting) the default widget it's generally easier to
 * call btk_widget_grab_focus() on the widget. Before making a widget
 * the default widget, you must set the #BTK_CAN_DEFAULT flag on the
 * widget you'd like to make the default using BTK_WIDGET_SET_FLAGS().
 **/
void
btk_window_set_default (BtkWindow *window,
			BtkWidget *default_widget)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (default_widget)
    g_return_if_fail (btk_widget_get_can_default (default_widget));
  
  if (window->default_widget != default_widget)
    {
      BtkWidget *old_default_widget = NULL;
      
      if (default_widget)
	g_object_ref (default_widget);
      
      if (window->default_widget)
	{
	  old_default_widget = window->default_widget;
	  
	  if (window->focus_widget != window->default_widget ||
	      !btk_widget_get_receives_default (window->default_widget))
            _btk_widget_set_has_default (window->default_widget, FALSE);
	  btk_widget_queue_draw (window->default_widget);
	}

      window->default_widget = default_widget;

      if (window->default_widget)
	{
	  if (window->focus_widget == NULL ||
	      !btk_widget_get_receives_default (window->focus_widget))
            _btk_widget_set_has_default (window->default_widget, TRUE);
	  btk_widget_queue_draw (window->default_widget);
	}

      if (old_default_widget)
	g_object_notify (G_OBJECT (old_default_widget), "has-default");
      
      if (default_widget)
	{
	  g_object_notify (G_OBJECT (default_widget), "has-default");
	  g_object_unref (default_widget);
	}
    }
}

/**
 * btk_window_get_default_widget:
 * @window: a #BtkWindow
 *
 * Returns the default widget for @window. See btk_window_set_default()
 * for more details.
 *
 * Returns: (transfer none): the default widget, or %NULL if there is none.
 *
 * Since: 2.14
 **/
BtkWidget *
btk_window_get_default_widget (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  return window->default_widget;
}

static void
btk_window_set_policy_internal (BtkWindow *window,
                                gboolean   allow_shrink,
                                gboolean   allow_grow,
                                gboolean   auto_shrink)
{
  window->allow_shrink = (allow_shrink != FALSE);
  window->allow_grow = (allow_grow != FALSE);

  g_object_freeze_notify (G_OBJECT (window));
  g_object_notify (G_OBJECT (window), "allow-shrink");
  g_object_notify (G_OBJECT (window), "allow-grow");
  g_object_notify (G_OBJECT (window), "resizable");
  g_object_thaw_notify (G_OBJECT (window));

  btk_widget_queue_resize_no_redraw (BTK_WIDGET (window));
}

void
btk_window_set_policy (BtkWindow *window,
		       gboolean   allow_shrink,
		       gboolean   allow_grow,
		       gboolean   auto_shrink)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  btk_window_set_policy_internal (window, allow_shrink, allow_grow, auto_shrink);
}

static gboolean
handle_keys_changed (gpointer data)
{
  BtkWindow *window;

  window = BTK_WINDOW (data);

  if (window->keys_changed_handler)
    {
      g_source_remove (window->keys_changed_handler);
      window->keys_changed_handler = 0;
    }

  g_signal_emit (window, window_signals[KEYS_CHANGED], 0);
  
  return FALSE;
}

static void
btk_window_notify_keys_changed (BtkWindow *window)
{
  if (!window->keys_changed_handler)
    window->keys_changed_handler = bdk_threads_add_idle (handle_keys_changed, window);
}

/**
 * btk_window_add_accel_group:
 * @window: window to attach accelerator group to
 * @accel_group: a #BtkAccelGroup
 *
 * Associate @accel_group with @window, such that calling
 * btk_accel_groups_activate() on @window will activate accelerators
 * in @accel_group.
 **/
void
btk_window_add_accel_group (BtkWindow     *window,
			    BtkAccelGroup *accel_group)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));

  _btk_accel_group_attach (accel_group, G_OBJECT (window));
  g_signal_connect_object (accel_group, "accel-changed",
			   G_CALLBACK (btk_window_notify_keys_changed),
			   window, G_CONNECT_SWAPPED);
  btk_window_notify_keys_changed (window);
}

/**
 * btk_window_remove_accel_group:
 * @window: a #BtkWindow
 * @accel_group: a #BtkAccelGroup
 *
 * Reverses the effects of btk_window_add_accel_group().
 **/
void
btk_window_remove_accel_group (BtkWindow     *window,
			       BtkAccelGroup *accel_group)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));

  g_signal_handlers_disconnect_by_func (accel_group,
					btk_window_notify_keys_changed,
					window);
  _btk_accel_group_detach (accel_group, G_OBJECT (window));
  btk_window_notify_keys_changed (window);
}

static BtkMnemonicHash *
btk_window_get_mnemonic_hash (BtkWindow *window,
			      gboolean   create)
{
  BtkWindowPrivate *private = BTK_WINDOW_GET_PRIVATE (window);
  if (!private->mnemonic_hash && create)
    private->mnemonic_hash = _btk_mnemonic_hash_new ();
  
  return private->mnemonic_hash;
}

/**
 * btk_window_add_mnemonic:
 * @window: a #BtkWindow
 * @keyval: the mnemonic
 * @target: the widget that gets activated by the mnemonic
 *
 * Adds a mnemonic to this window.
 */
void
btk_window_add_mnemonic (BtkWindow *window,
			 guint      keyval,
			 BtkWidget *target)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (BTK_IS_WIDGET (target));

  _btk_mnemonic_hash_add (btk_window_get_mnemonic_hash (window, TRUE),
			  keyval, target);
  btk_window_notify_keys_changed (window);
}

/**
 * btk_window_remove_mnemonic:
 * @window: a #BtkWindow
 * @keyval: the mnemonic
 * @target: the widget that gets activated by the mnemonic
 *
 * Removes a mnemonic from this window.
 */
void
btk_window_remove_mnemonic (BtkWindow *window,
			    guint      keyval,
			    BtkWidget *target)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (BTK_IS_WIDGET (target));
  
  _btk_mnemonic_hash_remove (btk_window_get_mnemonic_hash (window, TRUE),
			     keyval, target);
  btk_window_notify_keys_changed (window);
}

/**
 * btk_window_mnemonic_activate:
 * @window: a #BtkWindow
 * @keyval: the mnemonic
 * @modifier: the modifiers 
 * @returns: %TRUE if the activation is done. 
 * 
 * Activates the targets associated with the mnemonic.
 */
gboolean
btk_window_mnemonic_activate (BtkWindow      *window,
			      guint           keyval,
			      BdkModifierType modifier)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  if (window->mnemonic_modifier == (modifier & btk_accelerator_get_default_mod_mask ()))
      {
	BtkMnemonicHash *mnemonic_hash = btk_window_get_mnemonic_hash (window, FALSE);
	if (mnemonic_hash)
	  return _btk_mnemonic_hash_activate (mnemonic_hash, keyval);
      }

  return FALSE;
}

/**
 * btk_window_set_mnemonic_modifier:
 * @window: a #BtkWindow
 * @modifier: the modifier mask used to activate
 *               mnemonics on this window.
 *
 * Sets the mnemonic modifier for this window. 
 **/
void
btk_window_set_mnemonic_modifier (BtkWindow      *window,
				  BdkModifierType modifier)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail ((modifier & ~BDK_MODIFIER_MASK) == 0);

  window->mnemonic_modifier = modifier;
  btk_window_notify_keys_changed (window);
}

/**
 * btk_window_get_mnemonic_modifier:
 * @window: a #BtkWindow
 *
 * Returns the mnemonic modifier for this window. See
 * btk_window_set_mnemonic_modifier().
 *
 * Return value: the modifier mask used to activate
 *               mnemonics on this window.
 **/
BdkModifierType
btk_window_get_mnemonic_modifier (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), 0);

  return window->mnemonic_modifier;
}

/**
 * btk_window_set_position:
 * @window: a #BtkWindow.
 * @position: a position constraint.
 *
 * Sets a position constraint for this window. If the old or new
 * constraint is %BTK_WIN_POS_CENTER_ALWAYS, this will also cause
 * the window to be repositioned to satisfy the new constraint. 
 **/
void
btk_window_set_position (BtkWindow         *window,
			 BtkWindowPosition  position)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (position == BTK_WIN_POS_CENTER_ALWAYS ||
      window->position == BTK_WIN_POS_CENTER_ALWAYS)
    {
      BtkWindowGeometryInfo *info;

      info = btk_window_get_geometry_info (window, TRUE);

      /* this flag causes us to re-request the CENTER_ALWAYS
       * constraint in btk_window_move_resize(), see
       * comment in that function.
       */
      info->position_constraints_changed = TRUE;

      btk_widget_queue_resize_no_redraw (BTK_WIDGET (window));
    }

  window->position = position;
  
  g_object_notify (G_OBJECT (window), "window-position");
}

/**
 * btk_window_activate_focus:
 * @window: a #BtkWindow
 * 
 * Activates the current focused widget within the window.
 * 
 * Return value: %TRUE if a widget got activated.
 **/
gboolean 
btk_window_activate_focus (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  if (window->focus_widget && btk_widget_is_sensitive (window->focus_widget))
    return btk_widget_activate (window->focus_widget);

  return FALSE;
}

/**
 * btk_window_get_focus:
 * @window: a #BtkWindow
 * 
 * Retrieves the current focused widget within the window.
 * Note that this is the widget that would have the focus
 * if the toplevel window focused; if the toplevel window
 * is not focused then  <literal>btk_widget_has_focus (widget)</literal> will
 * not be %TRUE for the widget.
 *
 * Return value: (transfer none): the currently focused widget, or %NULL if there is none.
 **/
BtkWidget *
btk_window_get_focus (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  return window->focus_widget;
}

/**
 * btk_window_activate_default:
 * @window: a #BtkWindow
 * 
 * Activates the default widget for the window, unless the current 
 * focused widget has been configured to receive the default action 
 * (see btk_widget_set_receives_default()), in which case the
 * focused widget is activated. 
 * 
 * Return value: %TRUE if a widget got activated.
 **/
gboolean
btk_window_activate_default (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  if (window->default_widget && btk_widget_is_sensitive (window->default_widget) &&
      (!window->focus_widget || !btk_widget_get_receives_default (window->focus_widget)))
    return btk_widget_activate (window->default_widget);
  else if (window->focus_widget && btk_widget_is_sensitive (window->focus_widget))
    return btk_widget_activate (window->focus_widget);

  return FALSE;
}

/**
 * btk_window_set_modal:
 * @window: a #BtkWindow
 * @modal: whether the window is modal
 * 
 * Sets a window modal or non-modal. Modal windows prevent interaction
 * with other windows in the same application. To keep modal dialogs
 * on top of main application windows, use
 * btk_window_set_transient_for() to make the dialog transient for the
 * parent; most <link linkend="btk-X11-arch">window managers</link>
 * will then disallow lowering the dialog below the parent.
 * 
 * 
 **/
void
btk_window_set_modal (BtkWindow *window,
		      gboolean   modal)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_WINDOW (window));

  modal = modal != FALSE;
  if (window->modal == modal)
    return;
  
  window->modal = modal;
  widget = BTK_WIDGET (window);
  
  /* adjust desired modality state */
  if (btk_widget_get_realized (widget))
    {
      if (window->modal)
	bdk_window_set_modal_hint (widget->window, TRUE);
      else
	bdk_window_set_modal_hint (widget->window, FALSE);
    }

  if (btk_widget_get_visible (widget))
    {
      if (window->modal)
	btk_grab_add (widget);
      else
	btk_grab_remove (widget);
    }

  g_object_notify (G_OBJECT (window), "modal");
}

/**
 * btk_window_get_modal:
 * @window: a #BtkWindow
 * 
 * Returns whether the window is modal. See btk_window_set_modal().
 *
 * Return value: %TRUE if the window is set to be modal and
 *               establishes a grab when shown
 **/
gboolean
btk_window_get_modal (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  return window->modal;
}

/**
 * btk_window_list_toplevels:
 * 
 * Returns a list of all existing toplevel windows. The widgets
 * in the list are not individually referenced. If you want
 * to iterate through the list and perform actions involving
 * callbacks that might destroy the widgets, you <emphasis>must</emphasis> call
 * <literal>g_list_foreach (result, (GFunc)g_object_ref, NULL)</literal> first, and
 * then unref all the widgets afterwards.
 *
 * Return value: (element-type BtkWidget) (transfer container): list of toplevel widgets
 **/
GList*
btk_window_list_toplevels (void)
{
  GList *list = NULL;
  GSList *slist;

  for (slist = toplevel_list; slist; slist = slist->next)
    list = g_list_prepend (list, slist->data);

  return list;
}

void
btk_window_add_embedded_xid (BtkWindow *window, BdkNativeWindow xid)
{
  GList *embedded_windows;

  g_return_if_fail (BTK_IS_WINDOW (window));

  embedded_windows = g_object_get_qdata (G_OBJECT (window), quark_btk_embedded);
  if (embedded_windows)
    g_object_steal_qdata (G_OBJECT (window), quark_btk_embedded);
  embedded_windows = g_list_prepend (embedded_windows,
				     GUINT_TO_POINTER (xid));

  g_object_set_qdata_full (G_OBJECT (window), quark_btk_embedded, 
			   embedded_windows,
			   embedded_windows ?
			   (GDestroyNotify) g_list_free : NULL);
}

void
btk_window_remove_embedded_xid (BtkWindow *window, BdkNativeWindow xid)
{
  GList *embedded_windows;
  GList *node;

  g_return_if_fail (BTK_IS_WINDOW (window));
  
  embedded_windows = g_object_get_qdata (G_OBJECT (window), quark_btk_embedded);
  if (embedded_windows)
    g_object_steal_qdata (G_OBJECT (window), quark_btk_embedded);

  node = g_list_find (embedded_windows, GUINT_TO_POINTER (xid));
  if (node)
    {
      embedded_windows = g_list_remove_link (embedded_windows, node);
      g_list_free_1 (node);
    }
  
  g_object_set_qdata_full (G_OBJECT (window), quark_btk_embedded,
			   embedded_windows,
			   embedded_windows ?
			   (GDestroyNotify) g_list_free : NULL);
}

void       
_btk_window_reposition (BtkWindow *window,
			gint       x,
			gint       y)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  btk_window_move (window, x, y);
}

static void
btk_window_dispose (GObject *object)
{
  BtkWindow *window = BTK_WINDOW (object);

  btk_window_set_focus (window, NULL);
  btk_window_set_default (window, NULL);

  G_OBJECT_CLASS (btk_window_parent_class)->dispose (object);
}

static void
parent_destroyed_callback (BtkWindow *parent, BtkWindow *child)
{
  btk_widget_destroy (BTK_WIDGET (child));
}

static void
connect_parent_destroyed (BtkWindow *window)
{
  if (window->transient_parent)
    {
      g_signal_connect (window->transient_parent,
                        "destroy",
                        G_CALLBACK (parent_destroyed_callback),
                        window);
    }  
}

static void
disconnect_parent_destroyed (BtkWindow *window)
{
  if (window->transient_parent)
    {
      g_signal_handlers_disconnect_by_func (window->transient_parent,
					    parent_destroyed_callback,
					    window);
    }
}

static void
btk_window_transient_parent_realized (BtkWidget *parent,
				      BtkWidget *window)
{
  if (btk_widget_get_realized (BTK_WIDGET (window)))
    bdk_window_set_transient_for (window->window, parent->window);
}

static void
btk_window_transient_parent_unrealized (BtkWidget *parent,
					BtkWidget *window)
{
  if (btk_widget_get_realized (BTK_WIDGET (window)))
    bdk_property_delete (window->window, 
			 bdk_atom_intern_static_string ("WM_TRANSIENT_FOR"));
}

static void
btk_window_transient_parent_screen_changed (BtkWindow	*parent,
					    GParamSpec	*pspec,
					    BtkWindow   *window)
{
  btk_window_set_screen (window, parent->screen);
}

static void       
btk_window_unset_transient_for  (BtkWindow *window)
{
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (window);
  
  if (window->transient_parent)
    {
      g_signal_handlers_disconnect_by_func (window->transient_parent,
					    btk_window_transient_parent_realized,
					    window);
      g_signal_handlers_disconnect_by_func (window->transient_parent,
					    btk_window_transient_parent_unrealized,
					    window);
      g_signal_handlers_disconnect_by_func (window->transient_parent,
					    btk_window_transient_parent_screen_changed,
					    window);
      g_signal_handlers_disconnect_by_func (window->transient_parent,
					    btk_widget_destroyed,
					    &window->transient_parent);

      if (window->destroy_with_parent)
        disconnect_parent_destroyed (window);
      
      window->transient_parent = NULL;

      if (priv->transient_parent_group)
	{
	  priv->transient_parent_group = FALSE;
	  btk_window_group_remove_window (window->group,
					  window);
	}
    }
}

/**
 * btk_window_set_transient_for:
 * @window: a #BtkWindow
 * @parent: (allow-none): parent window, or %NULL
 *
 * Dialog windows should be set transient for the main application
 * window they were spawned from. This allows <link
 * linkend="btk-X11-arch">window managers</link> to e.g. keep the
 * dialog on top of the main window, or center the dialog over the
 * main window. btk_dialog_new_with_buttons() and other convenience
 * functions in BTK+ will sometimes call
 * btk_window_set_transient_for() on your behalf.
 *
 * Passing %NULL for @parent unsets the current transient window.
 *
 * On Windows, this function puts the child window on top of the parent,
 * much as the window manager would have done on X.
 */
void
btk_window_set_transient_for  (BtkWindow *window,
			       BtkWindow *parent)
{
  BtkWindowPrivate *priv;
  
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (parent == NULL || BTK_IS_WINDOW (parent));
  g_return_if_fail (window != parent);

  priv = BTK_WINDOW_GET_PRIVATE (window);

  if (window->transient_parent)
    {
      if (btk_widget_get_realized (BTK_WIDGET (window)) &&
          btk_widget_get_realized (BTK_WIDGET (window->transient_parent)) &&
          (!parent || !btk_widget_get_realized (BTK_WIDGET (parent))))
	btk_window_transient_parent_unrealized (BTK_WIDGET (window->transient_parent),
						BTK_WIDGET (window));

      btk_window_unset_transient_for (window);
    }

  window->transient_parent = parent;
  
  if (parent)
    {
      g_signal_connect (parent, "destroy",
			G_CALLBACK (btk_widget_destroyed),
			&window->transient_parent);
      g_signal_connect (parent, "realize",
			G_CALLBACK (btk_window_transient_parent_realized),
			window);
      g_signal_connect (parent, "unrealize",
			G_CALLBACK (btk_window_transient_parent_unrealized),
			window);
      g_signal_connect (parent, "notify::screen",
			G_CALLBACK (btk_window_transient_parent_screen_changed),
			window);
      
      btk_window_set_screen (window, parent->screen);

      if (window->destroy_with_parent)
        connect_parent_destroyed (window);
      
      if (btk_widget_get_realized (BTK_WIDGET (window)) &&
	  btk_widget_get_realized (BTK_WIDGET (parent)))
	btk_window_transient_parent_realized (BTK_WIDGET (parent),
					      BTK_WIDGET (window));

      if (parent->group)
	{
	  btk_window_group_add_window (parent->group, window);
	  priv->transient_parent_group = TRUE;
	}
    }
}

/**
 * btk_window_get_transient_for:
 * @window: a #BtkWindow
 *
 * Fetches the transient parent for this window. See
 * btk_window_set_transient_for().
 *
 * Return value: (transfer none): the transient parent for this window, or %NULL
 *    if no transient parent has been set.
 **/
BtkWindow *
btk_window_get_transient_for (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  return window->transient_parent;
}

/**
 * btk_window_set_opacity:
 * @window: a #BtkWindow
 * @opacity: desired opacity, between 0 and 1
 *
 * Request the windowing system to make @window partially transparent,
 * with opacity 0 being fully transparent and 1 fully opaque. (Values
 * of the opacity parameter are clamped to the [0,1] range.) On X11
 * this has any effect only on X screens with a compositing manager
 * running. See btk_widget_is_composited(). On Windows it should work
 * always.
 * 
 * Note that setting a window's opacity after the window has been
 * shown causes it to flicker once on Windows.
 *
 * Since: 2.12
 **/
void       
btk_window_set_opacity  (BtkWindow *window, 
			 gdouble    opacity)
{
  BtkWindowPrivate *priv;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  priv = BTK_WINDOW_GET_PRIVATE (window); 

  if (opacity < 0.0)
    opacity = 0.0;
  else if (opacity > 1.0)
    opacity = 1.0;

  priv->opacity_set = TRUE;
  priv->opacity = opacity;

  if (btk_widget_get_realized (BTK_WIDGET (window)))
    bdk_window_set_opacity (BTK_WIDGET (window)->window, priv->opacity);
}

/**
 * btk_window_get_opacity:
 * @window: a #BtkWindow
 *
 * Fetches the requested opacity for this window. See
 * btk_window_set_opacity().
 *
 * Return value: the requested opacity for this window.
 *
 * Since: 2.12
 **/
gdouble
btk_window_get_opacity (BtkWindow *window)
{
  BtkWindowPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_WINDOW (window), 0.0);

  priv = BTK_WINDOW_GET_PRIVATE (window); 

  return priv->opacity;
}

/**
 * btk_window_set_type_hint:
 * @window: a #BtkWindow
 * @hint: the window type
 *
 * By setting the type hint for the window, you allow the window
 * manager to decorate and handle the window in a way which is
 * suitable to the function of the window in your application.
 *
 * This function should be called before the window becomes visible.
 *
 * btk_dialog_new_with_buttons() and other convenience functions in BTK+
 * will sometimes call btk_window_set_type_hint() on your behalf.
 * 
 **/
void
btk_window_set_type_hint (BtkWindow           *window, 
			  BdkWindowTypeHint    hint)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (!btk_widget_get_mapped (BTK_WIDGET (window)));

  priv = BTK_WINDOW_GET_PRIVATE (window);

  if (hint < BDK_WINDOW_TYPE_HINT_DROPDOWN_MENU)
    window->type_hint = hint;
  else
    window->type_hint = BDK_WINDOW_TYPE_HINT_NORMAL;

  priv->reset_type_hint = TRUE;
  priv->type_hint = hint;
}

/**
 * btk_window_get_type_hint:
 * @window: a #BtkWindow
 *
 * Gets the type hint for this window. See btk_window_set_type_hint().
 *
 * Return value: the type hint for @window.
 **/
BdkWindowTypeHint
btk_window_get_type_hint (BtkWindow *window)
{
  BtkWindowPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_WINDOW (window), BDK_WINDOW_TYPE_HINT_NORMAL);

  priv = BTK_WINDOW_GET_PRIVATE (window);
  
  return priv->type_hint;
}

/**
 * btk_window_set_skip_taskbar_hint:
 * @window: a #BtkWindow 
 * @setting: %TRUE to keep this window from appearing in the task bar
 * 
 * Windows may set a hint asking the desktop environment not to display
 * the window in the task bar. This function sets this hint.
 * 
 * Since: 2.2
 **/
void
btk_window_set_skip_taskbar_hint (BtkWindow *window,
                                  gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (priv->skips_taskbar != setting)
    {
      priv->skips_taskbar = setting;
      if (btk_widget_get_realized (BTK_WIDGET (window)))
        bdk_window_set_skip_taskbar_hint (BTK_WIDGET (window)->window,
                                          priv->skips_taskbar);
      g_object_notify (G_OBJECT (window), "skip-taskbar-hint");
    }
}

/**
 * btk_window_get_skip_taskbar_hint:
 * @window: a #BtkWindow
 * 
 * Gets the value set by btk_window_set_skip_taskbar_hint()
 * 
 * Return value: %TRUE if window shouldn't be in taskbar
 * 
 * Since: 2.2
 **/
gboolean
btk_window_get_skip_taskbar_hint (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->skips_taskbar;
}

/**
 * btk_window_set_skip_pager_hint:
 * @window: a #BtkWindow 
 * @setting: %TRUE to keep this window from appearing in the pager
 * 
 * Windows may set a hint asking the desktop environment not to display
 * the window in the pager. This function sets this hint.
 * (A "pager" is any desktop navigation tool such as a workspace
 * switcher that displays a thumbnail representation of the windows
 * on the screen.)
 * 
 * Since: 2.2
 **/
void
btk_window_set_skip_pager_hint (BtkWindow *window,
                                gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (priv->skips_pager != setting)
    {
      priv->skips_pager = setting;
      if (btk_widget_get_realized (BTK_WIDGET (window)))
        bdk_window_set_skip_pager_hint (BTK_WIDGET (window)->window,
                                        priv->skips_pager);
      g_object_notify (G_OBJECT (window), "skip-pager-hint");
    }
}

/**
 * btk_window_get_skip_pager_hint:
 * @window: a #BtkWindow
 * 
 * Gets the value set by btk_window_set_skip_pager_hint().
 * 
 * Return value: %TRUE if window shouldn't be in pager
 * 
 * Since: 2.2
 **/
gboolean
btk_window_get_skip_pager_hint (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->skips_pager;
}

/**
 * btk_window_set_urgency_hint:
 * @window: a #BtkWindow 
 * @setting: %TRUE to mark this window as urgent
 * 
 * Windows may set a hint asking the desktop environment to draw
 * the users attention to the window. This function sets this hint.
 * 
 * Since: 2.8
 **/
void
btk_window_set_urgency_hint (BtkWindow *window,
			     gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (priv->urgent != setting)
    {
      priv->urgent = setting;
      if (btk_widget_get_realized (BTK_WIDGET (window)))
        bdk_window_set_urgency_hint (BTK_WIDGET (window)->window,
				     priv->urgent);
      g_object_notify (G_OBJECT (window), "urgency-hint");
    }
}

/**
 * btk_window_get_urgency_hint:
 * @window: a #BtkWindow
 * 
 * Gets the value set by btk_window_set_urgency_hint()
 * 
 * Return value: %TRUE if window is urgent
 * 
 * Since: 2.8
 **/
gboolean
btk_window_get_urgency_hint (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->urgent;
}

/**
 * btk_window_set_accept_focus:
 * @window: a #BtkWindow 
 * @setting: %TRUE to let this window receive input focus
 * 
 * Windows may set a hint asking the desktop environment not to receive
 * the input focus. This function sets this hint.
 * 
 * Since: 2.4
 **/
void
btk_window_set_accept_focus (BtkWindow *window,
			     gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (priv->accept_focus != setting)
    {
      priv->accept_focus = setting;
      if (btk_widget_get_realized (BTK_WIDGET (window)))
        bdk_window_set_accept_focus (BTK_WIDGET (window)->window,
				     priv->accept_focus);
      g_object_notify (G_OBJECT (window), "accept-focus");
    }
}

/**
 * btk_window_get_accept_focus:
 * @window: a #BtkWindow
 * 
 * Gets the value set by btk_window_set_accept_focus().
 * 
 * Return value: %TRUE if window should receive the input focus
 * 
 * Since: 2.4
 **/
gboolean
btk_window_get_accept_focus (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->accept_focus;
}

/**
 * btk_window_set_focus_on_map:
 * @window: a #BtkWindow 
 * @setting: %TRUE to let this window receive input focus on map
 * 
 * Windows may set a hint asking the desktop environment not to receive
 * the input focus when the window is mapped.  This function sets this
 * hint.
 * 
 * Since: 2.6
 **/
void
btk_window_set_focus_on_map (BtkWindow *window,
			     gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (priv->focus_on_map != setting)
    {
      priv->focus_on_map = setting;
      if (btk_widget_get_realized (BTK_WIDGET (window)))
        bdk_window_set_focus_on_map (BTK_WIDGET (window)->window,
				     priv->focus_on_map);
      g_object_notify (G_OBJECT (window), "focus-on-map");
    }
}

/**
 * btk_window_get_focus_on_map:
 * @window: a #BtkWindow
 * 
 * Gets the value set by btk_window_set_focus_on_map().
 * 
 * Return value: %TRUE if window should receive the input focus when
 * mapped.
 * 
 * Since: 2.6
 **/
gboolean
btk_window_get_focus_on_map (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  
  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->focus_on_map;
}

/**
 * btk_window_set_destroy_with_parent:
 * @window: a #BtkWindow
 * @setting: whether to destroy @window with its transient parent
 * 
 * If @setting is %TRUE, then destroying the transient parent of @window
 * will also destroy @window itself. This is useful for dialogs that
 * shouldn't persist beyond the lifetime of the main window they're
 * associated with, for example.
 **/
void
btk_window_set_destroy_with_parent  (BtkWindow *window,
                                     gboolean   setting)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (window->destroy_with_parent == (setting != FALSE))
    return;

  if (window->destroy_with_parent)
    {
      disconnect_parent_destroyed (window);
    }
  else
    {
      connect_parent_destroyed (window);
    }
  
  window->destroy_with_parent = setting;

  g_object_notify (G_OBJECT (window), "destroy-with-parent");
}

/**
 * btk_window_get_destroy_with_parent:
 * @window: a #BtkWindow
 * 
 * Returns whether the window will be destroyed with its transient parent. See
 * btk_window_set_destroy_with_parent ().
 *
 * Return value: %TRUE if the window will be destroyed with its transient parent.
 **/
gboolean
btk_window_get_destroy_with_parent (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  return window->destroy_with_parent;
}

static BtkWindowGeometryInfo*
btk_window_get_geometry_info (BtkWindow *window,
			      gboolean   create)
{
  BtkWindowGeometryInfo *info;

  info = window->geometry_info;
  if (!info && create)
    {
      info = g_new0 (BtkWindowGeometryInfo, 1);

      info->default_width = -1;
      info->default_height = -1;
      info->resize_width = -1;
      info->resize_height = -1;
      info->initial_x = 0;
      info->initial_y = 0;
      info->initial_pos_set = FALSE;
      info->default_is_geometry = FALSE;
      info->position_constraints_changed = FALSE;
      info->last.configure_request.x = 0;
      info->last.configure_request.y = 0;
      info->last.configure_request.width = -1;
      info->last.configure_request.height = -1;
      info->widget = NULL;
      info->mask = 0;
      window->geometry_info = info;
    }

  return info;
}

/**
 * btk_window_set_geometry_hints:
 * @window: a #BtkWindow
 * @geometry_widget: widget the geometry hints will be applied to
 * @geometry: struct containing geometry information
 * @geom_mask: mask indicating which struct fields should be paid attention to
 *
 * This function sets up hints about how a window can be resized by
 * the user.  You can set a minimum and maximum size; allowed resize
 * increments (e.g. for xterm, you can only resize by the size of a
 * character); aspect ratios; and more. See the #BdkGeometry struct.
 * 
 **/
void       
btk_window_set_geometry_hints (BtkWindow       *window,
			       BtkWidget       *geometry_widget,
			       BdkGeometry     *geometry,
			       BdkWindowHints   geom_mask)
{
  BtkWindowGeometryInfo *info;

  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (geometry_widget == NULL || BTK_IS_WIDGET (geometry_widget));

  info = btk_window_get_geometry_info (window, TRUE);
  
  if (info->widget)
    g_signal_handlers_disconnect_by_func (info->widget,
					  btk_widget_destroyed,
					  &info->widget);
  
  info->widget = geometry_widget;
  if (info->widget)
    g_signal_connect (geometry_widget, "destroy",
		      G_CALLBACK (btk_widget_destroyed),
		      &info->widget);

  if (geometry)
    info->geometry = *geometry;

  /* We store gravity in window->gravity not in the hints. */
  info->mask = geom_mask & ~(BDK_HINT_WIN_GRAVITY);

  if (geom_mask & BDK_HINT_WIN_GRAVITY)
    {
      btk_window_set_gravity (window, geometry->win_gravity);
    }
  
  btk_widget_queue_resize_no_redraw (BTK_WIDGET (window));
}

/**
 * btk_window_set_decorated:
 * @window: a #BtkWindow
 * @setting: %TRUE to decorate the window
 *
 * By default, windows are decorated with a title bar, resize
 * controls, etc.  Some <link linkend="btk-X11-arch">window
 * managers</link> allow BTK+ to disable these decorations, creating a
 * borderless window. If you set the decorated property to %FALSE
 * using this function, BTK+ will do its best to convince the window
 * manager not to decorate the window. Depending on the system, this
 * function may not have any effect when called on a window that is
 * already visible, so you should call it before calling btk_window_show().
 *
 * On Windows, this function always works, since there's no window manager
 * policy involved.
 * 
 **/
void
btk_window_set_decorated (BtkWindow *window,
                          gboolean   setting)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  setting = setting != FALSE;

  if (setting == window->decorated)
    return;

  window->decorated = setting;
  
  if (BTK_WIDGET (window)->window)
    {
      if (window->decorated)
        bdk_window_set_decorations (BTK_WIDGET (window)->window,
                                    BDK_DECOR_ALL);
      else
        bdk_window_set_decorations (BTK_WIDGET (window)->window,
                                    0);
    }

  g_object_notify (G_OBJECT (window), "decorated");
}

/**
 * btk_window_get_decorated:
 * @window: a #BtkWindow
 *
 * Returns whether the window has been set to have decorations
 * such as a title bar via btk_window_set_decorated().
 *
 * Return value: %TRUE if the window has been set to have decorations
 **/
gboolean
btk_window_get_decorated (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), TRUE);

  return window->decorated;
}

/**
 * btk_window_set_deletable:
 * @window: a #BtkWindow
 * @setting: %TRUE to decorate the window as deletable
 *
 * By default, windows have a close button in the window frame. Some 
 * <link linkend="btk-X11-arch">window managers</link> allow BTK+ to 
 * disable this button. If you set the deletable property to %FALSE
 * using this function, BTK+ will do its best to convince the window
 * manager not to show a close button. Depending on the system, this
 * function may not have any effect when called on a window that is
 * already visible, so you should call it before calling btk_window_show().
 *
 * On Windows, this function always works, since there's no window manager
 * policy involved.
 *
 * Since: 2.10
 */
void
btk_window_set_deletable (BtkWindow *window,
			  gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));

  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (setting == priv->deletable)
    return;

  priv->deletable = setting;
  
  if (BTK_WIDGET (window)->window)
    {
      if (priv->deletable)
        bdk_window_set_functions (BTK_WIDGET (window)->window,
				  BDK_FUNC_ALL);
      else
        bdk_window_set_functions (BTK_WIDGET (window)->window,
				  BDK_FUNC_ALL | BDK_FUNC_CLOSE);
    }

  g_object_notify (G_OBJECT (window), "deletable");  
}

/**
 * btk_window_get_deletable:
 * @window: a #BtkWindow
 *
 * Returns whether the window has been set to have a close button
 * via btk_window_set_deletable().
 *
 * Return value: %TRUE if the window has been set to have a close button
 *
 * Since: 2.10
 **/
gboolean
btk_window_get_deletable (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), TRUE);

  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->deletable;
}

static BtkWindowIconInfo*
get_icon_info (BtkWindow *window)
{
  return g_object_get_qdata (G_OBJECT (window), quark_btk_window_icon_info);
}
     
static void
free_icon_info (BtkWindowIconInfo *info)
{
  g_free (info->icon_name);
  g_slice_free (BtkWindowIconInfo, info);
}


static BtkWindowIconInfo*
ensure_icon_info (BtkWindow *window)
{
  BtkWindowIconInfo *info;

  info = get_icon_info (window);
  
  if (info == NULL)
    {
      info = g_slice_new0 (BtkWindowIconInfo);
      g_object_set_qdata_full (G_OBJECT (window),
                              quark_btk_window_icon_info,
                              info,
                              (GDestroyNotify)free_icon_info);
    }

  return info;
}

typedef struct {
  guint serial;
  BdkPixmap *pixmap;
  BdkPixmap *mask;
} ScreenIconInfo;

static ScreenIconInfo *
get_screen_icon_info (BdkScreen *screen)
{
  ScreenIconInfo *info = g_object_get_qdata (G_OBJECT (screen), 
					     quark_btk_window_default_icon_pixmap);
  if (!info)
    {
      info = g_slice_new0 (ScreenIconInfo);
      g_object_set_qdata (G_OBJECT (screen), 
			  quark_btk_window_default_icon_pixmap, info);
    }

  if (info->serial != default_icon_serial)
    {
      if (info->pixmap)
	{
	  g_object_remove_weak_pointer (G_OBJECT (info->pixmap), (gpointer*)&info->pixmap);
	  info->pixmap = NULL;
	}
	  
      if (info->mask)
	{
	  g_object_remove_weak_pointer (G_OBJECT (info->mask), (gpointer*)&info->mask);
	  info->mask = NULL;
	}

      info->serial = default_icon_serial;
    }
  
  return info;
}

static void
get_pixmap_and_mask (BdkWindow		*window,
		     BtkWindowIconInfo  *parent_info,
                     gboolean            is_default_list,
                     GList              *icon_list,
                     BdkPixmap         **pmap_return,
                     BdkBitmap         **mask_return)
{
  BdkScreen *screen = bdk_window_get_screen (window);
  ScreenIconInfo *default_icon_info = get_screen_icon_info (screen);
  BdkPixbuf *best_icon;
  GList *tmp_list;
  int best_size;
  
  *pmap_return = NULL;
  *mask_return = NULL;
  
  if (is_default_list &&
      default_icon_info->pixmap != NULL)
    {
      /* Use shared icon pixmap for all windows on this screen.
       */
      if (default_icon_info->pixmap)
        g_object_ref (default_icon_info->pixmap);
      if (default_icon_info->mask)
        g_object_ref (default_icon_info->mask);

      *pmap_return = default_icon_info->pixmap;
      *mask_return = default_icon_info->mask;
    }
  else if (parent_info && parent_info->icon_pixmap)
    {
      if (parent_info->icon_pixmap)
        g_object_ref (parent_info->icon_pixmap);
      if (parent_info->icon_mask)
        g_object_ref (parent_info->icon_mask);
      
      *pmap_return = parent_info->icon_pixmap;
      *mask_return = parent_info->icon_mask;
    }
  else
    {
#define IDEAL_SIZE 48
  
      best_size = G_MAXINT;
      best_icon = NULL;
      tmp_list = icon_list;
      while (tmp_list != NULL)
        {
          BdkPixbuf *pixbuf = tmp_list->data;
          int this;
      
          /* average width and height - if someone passes in a rectangular
           * icon they deserve what they get.
           */
          this = bdk_pixbuf_get_width (pixbuf) + bdk_pixbuf_get_height (pixbuf);
          this /= 2;
      
          if (best_icon == NULL)
            {
              best_icon = pixbuf;
              best_size = this;
            }
          else
            {
              /* icon is better if it's 32 pixels or larger, and closer to
               * the ideal size than the current best.
               */
              if (this >= 32 &&
                  (ABS (best_size - IDEAL_SIZE) <
                   ABS (this - IDEAL_SIZE)))
                {
                  best_icon = pixbuf;
                  best_size = this;
                }
            }

          tmp_list = tmp_list->next;
        }

      if (best_icon)
        bdk_pixbuf_render_pixmap_and_mask_for_colormap (best_icon,
							bdk_screen_get_system_colormap (screen),
							pmap_return,
							mask_return,
							128);

      /* Save pmap/mask for others to use if appropriate */
      if (parent_info)
        {
          parent_info->icon_pixmap = *pmap_return;
          parent_info->icon_mask = *mask_return;

          if (parent_info->icon_pixmap)
            g_object_ref (parent_info->icon_pixmap);
          if (parent_info->icon_mask)
            g_object_ref (parent_info->icon_mask);
        }
      else if (is_default_list)
        {
          default_icon_info->pixmap = *pmap_return;
          default_icon_info->mask = *mask_return;

          if (default_icon_info->pixmap)
	    g_object_add_weak_pointer (G_OBJECT (default_icon_info->pixmap),
				       (gpointer*)&default_icon_info->pixmap);
          if (default_icon_info->mask) 
	    g_object_add_weak_pointer (G_OBJECT (default_icon_info->mask),
				       (gpointer*)&default_icon_info->mask);
        }
    }
}

static GList *
icon_list_from_theme (BtkWidget    *widget,
		      const gchar  *name)
{
  GList *list;

  BtkIconTheme *icon_theme;
  BdkPixbuf *icon;
  gint *sizes;
  gint i;

  icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (widget));

  sizes = btk_icon_theme_get_icon_sizes (icon_theme, name);

  list = NULL;
  for (i = 0; sizes[i]; i++)
    {      
      /* FIXME
       * We need an EWMH extension to handle scalable icons 
       * by passing their name to the WM. For now just use a 
       * fixed size of 48.
       */ 
      if (sizes[i] == -1)
	icon = btk_icon_theme_load_icon (icon_theme, name,
					 48, 0, NULL);
      else
	icon = btk_icon_theme_load_icon (icon_theme, name,
					 sizes[i], 0, NULL);
      if (icon)
	list = g_list_append (list, icon);
    }

  g_free (sizes);

  return list;
}


static void
btk_window_realize_icon (BtkWindow *window)
{
  BtkWidget *widget;
  BtkWindowIconInfo *info;
  GList *icon_list;

  widget = BTK_WIDGET (window);

  g_return_if_fail (widget->window != NULL);

  /* no point setting an icon on override-redirect */
  if (window->type == BTK_WINDOW_POPUP)
    return;

  icon_list = NULL;
  
  info = ensure_icon_info (window);

  if (info->realized)
    return;

  g_return_if_fail (info->icon_pixmap == NULL);
  g_return_if_fail (info->icon_mask == NULL);
  
  info->using_default_icon = FALSE;
  info->using_parent_icon = FALSE;
  info->using_themed_icon = FALSE;
  
  icon_list = info->icon_list;

  /* Look up themed icon */
  if (icon_list == NULL && info->icon_name) 
    {
      icon_list = icon_list_from_theme (widget, info->icon_name);
      if (icon_list)
	info->using_themed_icon = TRUE;
    }

  /* Inherit from transient parent */
  if (icon_list == NULL && window->transient_parent)
    {
      icon_list = ensure_icon_info (window->transient_parent)->icon_list;
      if (icon_list)
        info->using_parent_icon = TRUE;
    }      

  /* Inherit from default */
  if (icon_list == NULL)
    {
      icon_list = default_icon_list;
      if (icon_list)
        info->using_default_icon = TRUE;
    }

  /* Look up themed icon */
  if (icon_list == NULL && default_icon_name) 
    {
      icon_list = icon_list_from_theme (widget, default_icon_name);
      info->using_default_icon = TRUE;
      info->using_themed_icon = TRUE;  
    }
  
  bdk_window_set_icon_list (widget->window, icon_list);

  get_pixmap_and_mask (widget->window,
		       info->using_parent_icon ? ensure_icon_info (window->transient_parent) : NULL,
                       info->using_default_icon,
                       icon_list,
                       &info->icon_pixmap,
                       &info->icon_mask);
  
  /* This is a slight ICCCM violation since it's a color pixmap not
   * a bitmap, but everyone does it.
   */
  bdk_window_set_icon (widget->window,
                       NULL,
                       info->icon_pixmap,
                       info->icon_mask);

  info->realized = TRUE;
  
  if (info->using_themed_icon) 
    {
      BtkIconTheme *icon_theme;

      g_list_foreach (icon_list, (GFunc) g_object_unref, NULL);
      g_list_free (icon_list);
 
      icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (window)));
      g_signal_connect (icon_theme, "changed",
			G_CALLBACK (update_themed_icon), window);
    }
}

static void
btk_window_unrealize_icon (BtkWindow *window)
{
  BtkWindowIconInfo *info;

  info = get_icon_info (window);

  if (info == NULL)
    return;
  
  if (info->icon_pixmap)
    g_object_unref (info->icon_pixmap);

  if (info->icon_mask)
    g_object_unref (info->icon_mask);

  info->icon_pixmap = NULL;
  info->icon_mask = NULL;

  if (info->using_themed_icon)
    {
      BtkIconTheme *icon_theme;

      icon_theme = btk_icon_theme_get_for_screen (btk_widget_get_screen (BTK_WIDGET (window)));

      g_signal_handlers_disconnect_by_func (icon_theme, update_themed_icon, window);
    }
    
  /* We don't clear the properties on the window, just figure the
   * window is going away.
   */

  info->realized = FALSE;

}

/**
 * btk_window_set_icon_list:
 * @window: a #BtkWindow
 * @list: list of #BdkPixbuf
 *
 * Sets up the icon representing a #BtkWindow. The icon is used when
 * the window is minimized (also known as iconified).  Some window
 * managers or desktop environments may also place it in the window
 * frame, or display it in other contexts.
 *
 * btk_window_set_icon_list() allows you to pass in the same icon in
 * several hand-drawn sizes. The list should contain the natural sizes
 * your icon is available in; that is, don't scale the image before
 * passing it to BTK+. Scaling is postponed until the last minute,
 * when the desired final size is known, to allow best quality.
 *
 * By passing several sizes, you may improve the final image quality
 * of the icon, by reducing or eliminating automatic image scaling.
 *
 * Recommended sizes to provide: 16x16, 32x32, 48x48 at minimum, and
 * larger images (64x64, 128x128) if you have them.
 *
 * See also btk_window_set_default_icon_list() to set the icon
 * for all windows in your application in one go.
 *
 * Note that transient windows (those who have been set transient for another
 * window using btk_window_set_transient_for()) will inherit their
 * icon from their transient parent. So there's no need to explicitly
 * set the icon on transient windows.
 **/
void
btk_window_set_icon_list (BtkWindow  *window,
                          GList      *list)
{
  BtkWindowIconInfo *info;

  g_return_if_fail (BTK_IS_WINDOW (window));

  info = ensure_icon_info (window);

  if (info->icon_list == list) /* check for NULL mostly */
    return;

  g_list_foreach (list,
                  (GFunc) g_object_ref, NULL);

  g_list_foreach (info->icon_list,
                  (GFunc) g_object_unref, NULL);

  g_list_free (info->icon_list);

  info->icon_list = g_list_copy (list);

  g_object_notify (G_OBJECT (window), "icon");
  
  btk_window_unrealize_icon (window);
  
  if (btk_widget_get_realized (BTK_WIDGET (window)))
    btk_window_realize_icon (window);

  /* We could try to update our transient children, but I don't think
   * it's really worth it. If we did it, the best way would probably
   * be to have children connect to notify::icon-list
   */
}

/**
 * btk_window_get_icon_list:
 * @window: a #BtkWindow
 * 
 * Retrieves the list of icons set by btk_window_set_icon_list().
 * The list is copied, but the reference count on each
 * member won't be incremented.
 *
 * Return value: (element-type BdkPixbuf) (transfer container): copy of window's icon list
 **/
GList*
btk_window_get_icon_list (BtkWindow  *window)
{
  BtkWindowIconInfo *info;
  
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  info = get_icon_info (window);

  if (info)
    return g_list_copy (info->icon_list);
  else
    return NULL;  
}

/**
 * btk_window_set_icon:
 * @window: a #BtkWindow
 * @icon: (allow-none): icon image, or %NULL
 *
 * Sets up the icon representing a #BtkWindow. This icon is used when
 * the window is minimized (also known as iconified).  Some window
 * managers or desktop environments may also place it in the window
 * frame, or display it in other contexts.
 *
 * The icon should be provided in whatever size it was naturally
 * drawn; that is, don't scale the image before passing it to
 * BTK+. Scaling is postponed until the last minute, when the desired
 * final size is known, to allow best quality.
 *
 * If you have your icon hand-drawn in multiple sizes, use
 * btk_window_set_icon_list(). Then the best size will be used.
 *
 * This function is equivalent to calling btk_window_set_icon_list()
 * with a 1-element list.
 *
 * See also btk_window_set_default_icon_list() to set the icon
 * for all windows in your application in one go.
 **/
void
btk_window_set_icon (BtkWindow  *window,
                     BdkPixbuf  *icon)
{
  GList *list;
  
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (icon == NULL || BDK_IS_PIXBUF (icon));

  list = NULL;

  if (icon)
    list = g_list_append (list, icon);
  
  btk_window_set_icon_list (window, list);
  g_list_free (list);  
}


static void 
update_themed_icon (BtkIconTheme *icon_theme,
		    BtkWindow    *window)
{
  g_object_notify (G_OBJECT (window), "icon");
  
  btk_window_unrealize_icon (window);
  
  if (btk_widget_get_realized (BTK_WIDGET (window)))
    btk_window_realize_icon (window);  
}

/**
 * btk_window_set_icon_name:
 * @window: a #BtkWindow
 * @name: (allow-none): the name of the themed icon
 *
 * Sets the icon for the window from a named themed icon. See
 * the docs for #BtkIconTheme for more details.
 *
 * Note that this has nothing to do with the WM_ICON_NAME 
 * property which is mentioned in the ICCCM.
 *
 * Since: 2.6
 */
void 
btk_window_set_icon_name (BtkWindow   *window,
			  const gchar *name)
{
  BtkWindowIconInfo *info;
  gchar *tmp;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  info = ensure_icon_info (window);

  if (g_strcmp0 (info->icon_name, name) == 0)
    return;

  tmp = info->icon_name;
  info->icon_name = g_strdup (name);
  g_free (tmp);

  g_list_foreach (info->icon_list, (GFunc) g_object_unref, NULL);
  g_list_free (info->icon_list);
  info->icon_list = NULL;
  
  update_themed_icon (NULL, window);

  g_object_notify (G_OBJECT (window), "icon-name");
}

/**
 * btk_window_get_icon_name:
 * @window: a #BtkWindow
 *
 * Returns the name of the themed icon for the window,
 * see btk_window_set_icon_name().
 *
 * Returns: the icon name or %NULL if the window has 
 * no themed icon
 *
 * Since: 2.6
 */
const gchar *
btk_window_get_icon_name (BtkWindow *window)
{
  BtkWindowIconInfo *info;

  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  info = ensure_icon_info (window);

  return info->icon_name;
}

/**
 * btk_window_get_icon:
 * @window: a #BtkWindow
 * 
 * Gets the value set by btk_window_set_icon() (or if you've
 * called btk_window_set_icon_list(), gets the first icon in
 * the icon list).
 *
 * Return value: (transfer none): icon for window
 **/
BdkPixbuf*
btk_window_get_icon (BtkWindow  *window)
{
  BtkWindowIconInfo *info;

  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);

  info = get_icon_info (window);
  if (info && info->icon_list)
    return BDK_PIXBUF (info->icon_list->data);
  else
    return NULL;
}

/* Load pixbuf, printing warning on failure if error == NULL
 */
static BdkPixbuf *
load_pixbuf_verbosely (const char *filename,
		       GError    **err)
{
  GError *local_err = NULL;
  BdkPixbuf *pixbuf;

  pixbuf = bdk_pixbuf_new_from_file (filename, &local_err);

  if (!pixbuf)
    {
      if (err)
	*err = local_err;
      else
	{
	  g_warning ("Error loading icon from file '%s':\n\t%s",
		     filename, local_err->message);
	  g_error_free (local_err);
	}
    }

  return pixbuf;
}

/**
 * btk_window_set_icon_from_file:
 * @window: a #BtkWindow
 * @filename: location of icon file
 * @err: (allow-none): location to store error, or %NULL.
 *
 * Sets the icon for @window.  
 * Warns on failure if @err is %NULL.
 *
 * This function is equivalent to calling btk_window_set_icon()
 * with a pixbuf created by loading the image from @filename.
 *
 * Returns: %TRUE if setting the icon succeeded.
 *
 * Since: 2.2
 **/
gboolean
btk_window_set_icon_from_file (BtkWindow   *window,
			       const gchar *filename,
			       GError     **err)
{
  BdkPixbuf *pixbuf = load_pixbuf_verbosely (filename, err);

  if (pixbuf)
    {
      btk_window_set_icon (window, pixbuf);
      g_object_unref (pixbuf);
      
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_window_set_default_icon_list:
 * @list: (element-type BdkPixbuf) (transfer container): a list of #BdkPixbuf
 *
 * Sets an icon list to be used as fallback for windows that haven't
 * had btk_window_set_icon_list() called on them to set up a
 * window-specific icon list. This function allows you to set up the
 * icon for all windows in your app at once.
 *
 * See btk_window_set_icon_list() for more details.
 * 
 **/
void
btk_window_set_default_icon_list (GList *list)
{
  GList *toplevels;
  GList *tmp_list;
  if (list == default_icon_list)
    return;

  /* Update serial so we don't used cached pixmaps/masks
   */
  default_icon_serial++;
  
  g_list_foreach (list,
                  (GFunc) g_object_ref, NULL);

  g_list_foreach (default_icon_list,
                  (GFunc) g_object_unref, NULL);

  g_list_free (default_icon_list);

  default_icon_list = g_list_copy (list);
  
  /* Update all toplevels */
  toplevels = btk_window_list_toplevels ();
  tmp_list = toplevels;
  while (tmp_list != NULL)
    {
      BtkWindowIconInfo *info;
      BtkWindow *w = tmp_list->data;
      
      info = get_icon_info (w);
      if (info && info->using_default_icon)
        {
          btk_window_unrealize_icon (w);
          if (btk_widget_get_realized (BTK_WIDGET (w)))
            btk_window_realize_icon (w);
        }

      tmp_list = tmp_list->next;
    }
  g_list_free (toplevels);
}

/**
 * btk_window_set_default_icon:
 * @icon: the icon
 *
 * Sets an icon to be used as fallback for windows that haven't
 * had btk_window_set_icon() called on them from a pixbuf.
 *
 * Since: 2.4
 **/
void
btk_window_set_default_icon (BdkPixbuf *icon)
{
  GList *list;
  
  g_return_if_fail (BDK_IS_PIXBUF (icon));

  list = g_list_prepend (NULL, icon);
  btk_window_set_default_icon_list (list);
  g_list_free (list);
}

/**
 * btk_window_set_default_icon_name:
 * @name: the name of the themed icon
 *
 * Sets an icon to be used as fallback for windows that haven't
 * had btk_window_set_icon_list() called on them from a named
 * themed icon, see btk_window_set_icon_name().
 *
 * Since: 2.6
 **/
void
btk_window_set_default_icon_name (const gchar *name)
{
  GList *tmp_list;
  GList *toplevels;

  /* Update serial so we don't used cached pixmaps/masks
   */
  default_icon_serial++;

  g_free (default_icon_name);
  default_icon_name = g_strdup (name);

  g_list_foreach (default_icon_list,
                  (GFunc) g_object_unref, NULL);

  g_list_free (default_icon_list);
  default_icon_list = NULL;
  
  /* Update all toplevels */
  toplevels = btk_window_list_toplevels ();
  tmp_list = toplevels;
  while (tmp_list != NULL)
    {
      BtkWindowIconInfo *info;
      BtkWindow *w = tmp_list->data;
      
      info = get_icon_info (w);
      if (info && info->using_default_icon && info->using_themed_icon)
        {
          btk_window_unrealize_icon (w);
          if (btk_widget_get_realized (BTK_WIDGET (w)))
            btk_window_realize_icon (w);
        }

      tmp_list = tmp_list->next;
    }
  g_list_free (toplevels);
}

/**
 * btk_window_get_default_icon_name:
 *
 * Returns the fallback icon name for windows that has been set
 * with btk_window_set_default_icon_name(). The returned
 * string is owned by BTK+ and should not be modified. It
 * is only valid until the next call to
 * btk_window_set_default_icon_name().
 *
 * Returns: the fallback icon name for windows
 *
 * Since: 2.16
 */
const gchar *
btk_window_get_default_icon_name (void)
{
  return default_icon_name;
}

/**
 * btk_window_set_default_icon_from_file:
 * @filename: location of icon file
 * @err: (allow-none): location to store error, or %NULL.
 *
 * Sets an icon to be used as fallback for windows that haven't
 * had btk_window_set_icon_list() called on them from a file
 * on disk. Warns on failure if @err is %NULL.
 *
 * Returns: %TRUE if setting the icon succeeded.
 *
 * Since: 2.2
 **/
gboolean
btk_window_set_default_icon_from_file (const gchar *filename,
				       GError     **err)
{
  BdkPixbuf *pixbuf = load_pixbuf_verbosely (filename, err);

  if (pixbuf)
    {
      btk_window_set_default_icon (pixbuf);
      g_object_unref (pixbuf);
      
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_window_get_default_icon_list:
 * 
 * Gets the value set by btk_window_set_default_icon_list().
 * The list is a copy and should be freed with g_list_free(),
 * but the pixbufs in the list have not had their reference count
 * incremented.
 * 
 * Return value: (element-type BdkPixbuf) (transfer container): copy of default icon list 
 **/
GList*
btk_window_get_default_icon_list (void)
{
  return g_list_copy (default_icon_list);
}

static void
btk_window_set_default_size_internal (BtkWindow    *window,
                                      gboolean      change_width,
                                      gint          width,
                                      gboolean      change_height,
                                      gint          height,
				      gboolean      is_geometry)
{
  BtkWindowGeometryInfo *info;

  g_return_if_fail (change_width == FALSE || width >= -1);
  g_return_if_fail (change_height == FALSE || height >= -1);

  info = btk_window_get_geometry_info (window, TRUE);

  g_object_freeze_notify (G_OBJECT (window));

  info->default_is_geometry = is_geometry != FALSE;

  if (change_width)
    {
      if (width == 0)
        width = 1;

      if (width < 0)
        width = -1;

      info->default_width = width;

      g_object_notify (G_OBJECT (window), "default-width");
    }

  if (change_height)
    {
      if (height == 0)
        height = 1;

      if (height < 0)
        height = -1;

      info->default_height = height;
      
      g_object_notify (G_OBJECT (window), "default-height");
    }
  
  g_object_thaw_notify (G_OBJECT (window));
  
  btk_widget_queue_resize_no_redraw (BTK_WIDGET (window));
}

/**
 * btk_window_set_default_size:
 * @window: a #BtkWindow
 * @width: width in pixels, or -1 to unset the default width
 * @height: height in pixels, or -1 to unset the default height
 *
 * Sets the default size of a window. If the window's "natural" size
 * (its size request) is larger than the default, the default will be
 * ignored. More generally, if the default size does not obey the
 * geometry hints for the window (btk_window_set_geometry_hints() can
 * be used to set these explicitly), the default size will be clamped
 * to the nearest permitted size.
 * 
 * Unlike btk_widget_set_size_request(), which sets a size request for
 * a widget and thus would keep users from shrinking the window, this
 * function only sets the initial size, just as if the user had
 * resized the window themselves. Users can still shrink the window
 * again as they normally would. Setting a default size of -1 means to
 * use the "natural" default size (the size request of the window).
 *
 * For more control over a window's initial size and how resizing works,
 * investigate btk_window_set_geometry_hints().
 *
 * For some uses, btk_window_resize() is a more appropriate function.
 * btk_window_resize() changes the current size of the window, rather
 * than the size to be used on initial display. btk_window_resize() always
 * affects the window itself, not the geometry widget.
 *
 * The default size of a window only affects the first time a window is
 * shown; if a window is hidden and re-shown, it will remember the size
 * it had prior to hiding, rather than using the default size.
 *
 * Windows can't actually be 0x0 in size, they must be at least 1x1, but
 * passing 0 for @width and @height is OK, resulting in a 1x1 default size.
 **/
void       
btk_window_set_default_size (BtkWindow   *window,
			     gint         width,
			     gint         height)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  btk_window_set_default_size_internal (window, TRUE, width, TRUE, height, FALSE);
}

/**
 * btk_window_get_default_size:
 * @window: a #BtkWindow
 * @width: (allow-none): location to store the default width, or %NULL
 * @height: (allow-none): location to store the default height, or %NULL
 *
 * Gets the default size of the window. A value of -1 for the width or
 * height indicates that a default size has not been explicitly set
 * for that dimension, so the "natural" size of the window will be
 * used.
 * 
 **/
void
btk_window_get_default_size (BtkWindow *window,
			     gint      *width,
			     gint      *height)
{
  BtkWindowGeometryInfo *info;

  g_return_if_fail (BTK_IS_WINDOW (window));

  info = btk_window_get_geometry_info (window, FALSE);

  if (width)
    *width = info ? info->default_width : -1;

  if (height)
    *height = info ? info->default_height : -1;
}

/**
 * btk_window_resize:
 * @window: a #BtkWindow
 * @width: width in pixels to resize the window to
 * @height: height in pixels to resize the window to
 *
 * Resizes the window as if the user had done so, obeying geometry
 * constraints. The default geometry constraint is that windows may
 * not be smaller than their size request; to override this
 * constraint, call btk_widget_set_size_request() to set the window's
 * request to a smaller value.
 *
 * If btk_window_resize() is called before showing a window for the
 * first time, it overrides any default size set with
 * btk_window_set_default_size().
 *
 * Windows may not be resized smaller than 1 by 1 pixels.
 * 
 **/
void
btk_window_resize (BtkWindow *window,
                   gint       width,
                   gint       height)
{
  BtkWindowGeometryInfo *info;
  
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  info = btk_window_get_geometry_info (window, TRUE);

  info->resize_width = width;
  info->resize_height = height;

  btk_widget_queue_resize_no_redraw (BTK_WIDGET (window));
}

/**
 * btk_window_get_size:
 * @window: a #BtkWindow
 * @width: (out) (allow-none): return location for width, or %NULL
 * @height: (out) (allow-none): return location for height, or %NULL
 *
 * Obtains the current size of @window. If @window is not onscreen,
 * it returns the size BTK+ will suggest to the <link
 * linkend="btk-X11-arch">window manager</link> for the initial window
 * size (but this is not reliably the same as the size the window
 * manager will actually select). The size obtained by
 * btk_window_get_size() is the last size received in a
 * #BdkEventConfigure, that is, BTK+ uses its locally-stored size,
 * rather than querying the X server for the size. As a result, if you
 * call btk_window_resize() then immediately call
 * btk_window_get_size(), the size won't have taken effect yet. After
 * the window manager processes the resize request, BTK+ receives
 * notification that the size has changed via a configure event, and
 * the size of the window gets updated.
 *
 * Note 1: Nearly any use of this function creates a race condition,
 * because the size of the window may change between the time that you
 * get the size and the time that you perform some action assuming
 * that size is the current size. To avoid race conditions, connect to
 * "configure-event" on the window and adjust your size-dependent
 * state to match the size delivered in the #BdkEventConfigure.
 *
 * Note 2: The returned size does <emphasis>not</emphasis> include the
 * size of the window manager decorations (aka the window frame or
 * border). Those are not drawn by BTK+ and BTK+ has no reliable
 * method of determining their size.
 *
 * Note 3: If you are getting a window size in order to position
 * the window onscreen, there may be a better way. The preferred
 * way is to simply set the window's semantic type with
 * btk_window_set_type_hint(), which allows the window manager to
 * e.g. center dialogs. Also, if you set the transient parent of
 * dialogs with btk_window_set_transient_for() window managers
 * will often center the dialog over its parent window. It's
 * much preferred to let the window manager handle these
 * things rather than doing it yourself, because all apps will
 * behave consistently and according to user prefs if the window
 * manager handles it. Also, the window manager can take the size
 * of the window decorations/border into account, while your
 * application cannot.
 *
 * In any case, if you insist on application-specified window
 * positioning, there's <emphasis>still</emphasis> a better way than
 * doing it yourself - btk_window_set_position() will frequently
 * handle the details for you.
 * 
 **/
void
btk_window_get_size (BtkWindow *window,
                     gint      *width,
                     gint      *height)
{
  gint w, h;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (width == NULL && height == NULL)
    return;

  if (btk_widget_get_mapped (BTK_WIDGET (window)))
    {
      w = bdk_window_get_width (BTK_WIDGET (window)->window);
      h = bdk_window_get_height (BTK_WIDGET (window)->window);
    }
  else
    {
      BdkRectangle configure_request;

      btk_window_compute_configure_request (window,
                                            &configure_request,
                                            NULL, NULL);

      w = configure_request.width;
      h = configure_request.height;
    }
  
  if (width)
    *width = w;
  if (height)
    *height = h;
}

/**
 * btk_window_move:
 * @window: a #BtkWindow
 * @x: X coordinate to move window to
 * @y: Y coordinate to move window to
 *
 * Asks the <link linkend="btk-X11-arch">window manager</link> to move
 * @window to the given position.  Window managers are free to ignore
 * this; most window managers ignore requests for initial window
 * positions (instead using a user-defined placement algorithm) and
 * honor requests after the window has already been shown.
 *
 * Note: the position is the position of the gravity-determined
 * reference point for the window. The gravity determines two things:
 * first, the location of the reference point in root window
 * coordinates; and second, which point on the window is positioned at
 * the reference point.
 *
 * By default the gravity is #BDK_GRAVITY_NORTH_WEST, so the reference
 * point is simply the @x, @y supplied to btk_window_move(). The
 * top-left corner of the window decorations (aka window frame or
 * border) will be placed at @x, @y.  Therefore, to position a window
 * at the top left of the screen, you want to use the default gravity
 * (which is #BDK_GRAVITY_NORTH_WEST) and move the window to 0,0.
 *
 * To position a window at the bottom right corner of the screen, you
 * would set #BDK_GRAVITY_SOUTH_EAST, which means that the reference
 * point is at @x + the window width and @y + the window height, and
 * the bottom-right corner of the window border will be placed at that
 * reference point. So, to place a window in the bottom right corner
 * you would first set gravity to south east, then write:
 * <literal>btk_window_move (window, bdk_screen_width () - window_width,
 * bdk_screen_height () - window_height)</literal> (note that this
 * example does not take multi-head scenarios into account).
 *
 * The Extended Window Manager Hints specification at <ulink 
 * url="http://www.freedesktop.org/Standards/wm-spec">
 * http://www.freedesktop.org/Standards/wm-spec</ulink> has a 
 * nice table of gravities in the "implementation notes" section.
 *
 * The btk_window_get_position() documentation may also be relevant.
 */
void
btk_window_move (BtkWindow *window,
                 gint       x,
                 gint       y)
{
  BtkWindowGeometryInfo *info;
  BtkWidget *widget;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  info = btk_window_get_geometry_info (window, TRUE);  
  
  if (btk_widget_get_mapped (widget))
    {
      /* we have now sent a request with this position
       * with currently-active constraints, so toggle flag.
       */
      info->position_constraints_changed = FALSE;

      /* we only constrain if mapped - if not mapped,
       * then btk_window_compute_configure_request()
       * will apply the constraints later, and we
       * don't want to lose information about
       * what position the user set before then.
       * i.e. if you do a move() then turn off POS_CENTER
       * then show the window, your move() will work.
       */
      btk_window_constrain_position (window,
                                     widget->allocation.width,
                                     widget->allocation.height,
                                     &x, &y);
      
      /* Note that this request doesn't go through our standard request
       * framework, e.g. doesn't increment configure_request_count,
       * doesn't set info->last, etc.; that's because
       * we don't save the info needed to arrive at this same request
       * again.
       *
       * To btk_window_move_resize(), this will end up looking exactly
       * the same as the position being changed by the window
       * manager.
       */
      
      /* FIXME are we handling gravity properly for framed windows? */
      if (window->frame)
        bdk_window_move (window->frame,
                         x - window->frame_left,
                         y - window->frame_top);
      else
        bdk_window_move (BTK_WIDGET (window)->window,
                         x, y);
    }
  else
    {
      /* Save this position to apply on mapping */
      info->initial_x = x;
      info->initial_y = y;
      info->initial_pos_set = TRUE;
    }
}

/**
 * btk_window_get_position:
 * @window: a #BtkWindow
 * @root_x: (out) (allow-none): return location for X coordinate of gravity-determined reference point
 * @root_y: (out) (allow-none): return location for Y coordinate of gravity-determined reference point
 *
 * This function returns the position you need to pass to
 * btk_window_move() to keep @window in its current position.  This
 * means that the meaning of the returned value varies with window
 * gravity. See btk_window_move() for more details.
 * 
 * If you haven't changed the window gravity, its gravity will be
 * #BDK_GRAVITY_NORTH_WEST. This means that btk_window_get_position()
 * gets the position of the top-left corner of the window manager
 * frame for the window. btk_window_move() sets the position of this
 * same top-left corner.
 *
 * btk_window_get_position() is not 100% reliable because the X Window System
 * does not specify a way to obtain the geometry of the
 * decorations placed on a window by the window manager.
 * Thus BTK+ is using a "best guess" that works with most
 * window managers.
 *
 * Moreover, nearly all window managers are historically broken with
 * respect to their handling of window gravity. So moving a window to
 * its current position as returned by btk_window_get_position() tends
 * to result in moving the window slightly. Window managers are
 * slowly getting better over time.
 *
 * If a window has gravity #BDK_GRAVITY_STATIC the window manager
 * frame is not relevant, and thus btk_window_get_position() will
 * always produce accurate results. However you can't use static
 * gravity to do things like place a window in a corner of the screen,
 * because static gravity ignores the window manager decorations.
 *
 * If you are saving and restoring your application's window
 * positions, you should know that it's impossible for applications to
 * do this without getting it somewhat wrong because applications do
 * not have sufficient knowledge of window manager state. The Correct
 * Mechanism is to support the session management protocol (see the
 * "BunnyClient" object in the BUNNY libraries for example) and allow
 * the window manager to save your window sizes and positions.
 * 
 **/

void
btk_window_get_position (BtkWindow *window,
                         gint      *root_x,
                         gint      *root_y)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);
  
  if (window->gravity == BDK_GRAVITY_STATIC)
    {
      if (btk_widget_get_mapped (widget))
        {
          /* This does a server round-trip, which is sort of wrong;
           * but a server round-trip is inevitable for
           * bdk_window_get_frame_extents() in the usual
           * NorthWestGravity case below, so not sure what else to
           * do. We should likely be consistent about whether we get
           * the client-side info or the server-side info.
           */
          bdk_window_get_origin (widget->window, root_x, root_y);
        }
      else
        {
          BdkRectangle configure_request;
          
          btk_window_compute_configure_request (window,
                                                &configure_request,
                                                NULL, NULL);
          
          *root_x = configure_request.x;
          *root_y = configure_request.y;
        }
    }
  else
    {
      BdkRectangle frame_extents;
      
      gint x, y;
      gint w, h;
      
      if (btk_widget_get_mapped (widget))
        {
	  if (window->frame)
	    bdk_window_get_frame_extents (window->frame, &frame_extents);
	  else
	    bdk_window_get_frame_extents (widget->window, &frame_extents);
          x = frame_extents.x;
          y = frame_extents.y;
          btk_window_get_size (window, &w, &h);
        }
      else
        {
          /* We just say the frame has 0 size on all sides.
           * Not sure what else to do.
           */             
          btk_window_compute_configure_request (window,
                                                &frame_extents,
                                                NULL, NULL);
          x = frame_extents.x;
          y = frame_extents.y;
          w = frame_extents.width;
          h = frame_extents.height;
        }
      
      switch (window->gravity)
        {
        case BDK_GRAVITY_NORTH:
        case BDK_GRAVITY_CENTER:
        case BDK_GRAVITY_SOUTH:
          /* Find center of frame. */
          x += frame_extents.width / 2;
          /* Center client window on that point. */
          x -= w / 2;
          break;

        case BDK_GRAVITY_SOUTH_EAST:
        case BDK_GRAVITY_EAST:
        case BDK_GRAVITY_NORTH_EAST:
          /* Find right edge of frame */
          x += frame_extents.width;
          /* Align left edge of client at that point. */
          x -= w;
          break;
        default:
          break;
        }

      switch (window->gravity)
        {
        case BDK_GRAVITY_WEST:
        case BDK_GRAVITY_CENTER:
        case BDK_GRAVITY_EAST:
          /* Find center of frame. */
          y += frame_extents.height / 2;
          /* Center client window there. */
          y -= h / 2;
          break;
        case BDK_GRAVITY_SOUTH_WEST:
        case BDK_GRAVITY_SOUTH:
        case BDK_GRAVITY_SOUTH_EAST:
          /* Find south edge of frame */
          y += frame_extents.height;
          /* Place bottom edge of client there */
          y -= h;
          break;
        default:
          break;
        }
      
      if (root_x)
        *root_x = x;
      if (root_y)
        *root_y = y;
    }
}

/**
 * btk_window_reshow_with_initial_size:
 * @window: a #BtkWindow
 * 
 * Hides @window, then reshows it, resetting the
 * default size and position of the window. Used
 * by GUI builders only.
 **/
void
btk_window_reshow_with_initial_size (BtkWindow *window)
{
  BtkWidget *widget;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);
  
  btk_widget_hide (widget);
  btk_widget_unrealize (widget);
  btk_widget_show (widget);
}

static void
btk_window_destroy (BtkObject *object)
{
  BtkWindow *window = BTK_WINDOW (object);
  
  toplevel_list = g_slist_remove (toplevel_list, window);

  if (window->transient_parent)
    btk_window_set_transient_for (window, NULL);

  /* frees the icons */
  btk_window_set_icon_list (window, NULL);
  
  if (window->has_user_ref_count)
    {
      window->has_user_ref_count = FALSE;
      g_object_unref (window);
    }

  if (window->group)
    btk_window_group_remove_window (window->group, window);

   btk_window_free_key_hash (window);

   BTK_OBJECT_CLASS (btk_window_parent_class)->destroy (object);
}

static void
btk_window_finalize (GObject *object)
{
  BtkWindow *window = BTK_WINDOW (object);
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (window);
  BtkMnemonicHash *mnemonic_hash;

  g_free (window->title);
  g_free (window->wmclass_name);
  g_free (window->wmclass_class);
  g_free (window->wm_role);

  mnemonic_hash = btk_window_get_mnemonic_hash (window, FALSE);
  if (mnemonic_hash)
    _btk_mnemonic_hash_free (mnemonic_hash);

  if (window->geometry_info)
    {
      if (window->geometry_info->widget)
	g_signal_handlers_disconnect_by_func (window->geometry_info->widget,
					      btk_widget_destroyed,
					      &window->geometry_info->widget);
      g_free (window->geometry_info);
    }

  if (window->keys_changed_handler)
    {
      g_source_remove (window->keys_changed_handler);
      window->keys_changed_handler = 0;
    }

  if (window->screen)
    g_signal_handlers_disconnect_by_func (window->screen,
                                          btk_window_on_composited_changed, window);

  g_free (priv->startup_id);

  G_OBJECT_CLASS (btk_window_parent_class)->finalize (object);
}

static void
btk_window_show (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkContainer *container = BTK_CONTAINER (window);
  gboolean need_resize;

  BTK_WIDGET_SET_FLAGS (widget, BTK_VISIBLE);
  
  need_resize = container->need_resize || !btk_widget_get_realized (widget);
  container->need_resize = FALSE;

  if (need_resize)
    {
      BtkWindowGeometryInfo *info = btk_window_get_geometry_info (window, TRUE);
      BtkAllocation allocation = { 0, 0 };
      BdkRectangle configure_request;
      BdkGeometry new_geometry;
      guint new_flags;
      gboolean was_realized;

      /* We are going to go ahead and perform this configure request
       * and then emulate a configure notify by going ahead and
       * doing a size allocate. Sort of a synchronous
       * mini-copy of btk_window_move_resize() here.
       */
      btk_window_compute_configure_request (window,
                                            &configure_request,
                                            &new_geometry,
                                            &new_flags);
      
      /* We update this because we are going to go ahead
       * and bdk_window_resize() below, rather than
       * queuing it.
       */
      info->last.configure_request.width = configure_request.width;
      info->last.configure_request.height = configure_request.height;
      
      /* and allocate the window - this is normally done
       * in move_resize in response to configure notify
       */
      allocation.width  = configure_request.width;
      allocation.height = configure_request.height;
      btk_widget_size_allocate (widget, &allocation);

      /* Then we guarantee we have a realize */
      was_realized = FALSE;
      if (!btk_widget_get_realized (widget))
	{
	  btk_widget_realize (widget);
	  was_realized = TRUE;
	}

      /* Must be done after the windows are realized,
       * so that the decorations can be read
       */
      btk_decorated_window_calculate_frame_size (window);

      /* We only send configure request if we didn't just finish
       * creating the window; if we just created the window
       * then we created it with widget->allocation anyhow.
       */
      if (!was_realized)
	bdk_window_move_resize (widget->window,
				configure_request.x,
				configure_request.y,
				configure_request.width,
				configure_request.height);
    }
  
  btk_container_check_resize (container);

  btk_widget_map (widget);

  /* Try to make sure that we have some focused widget
   */
  if (!window->focus_widget && !BTK_IS_PLUG (window))
    btk_window_move_focus (window, BTK_DIR_TAB_FORWARD);
  
  if (window->modal)
    btk_grab_add (widget);
}

static void
btk_window_hide (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);

  BTK_WIDGET_UNSET_FLAGS (widget, BTK_VISIBLE);
  btk_widget_unmap (widget);

  if (window->modal)
    btk_grab_remove (widget);
}

static void
btk_window_map (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (window);
  BdkWindow *toplevel;
  gboolean auto_mnemonics;

  btk_widget_set_mapped (widget, TRUE);

  if (window->bin.child &&
      btk_widget_get_visible (window->bin.child) &&
      !btk_widget_get_mapped (window->bin.child))
    btk_widget_map (window->bin.child);

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (window->maximize_initially)
    bdk_window_maximize (toplevel);
  else
    bdk_window_unmaximize (toplevel);
  
  if (window->stick_initially)
    bdk_window_stick (toplevel);
  else
    bdk_window_unstick (toplevel);
  
  if (window->iconify_initially)
    bdk_window_iconify (toplevel);
  else
    bdk_window_deiconify (toplevel);

  if (priv->fullscreen_initially)
    bdk_window_fullscreen (toplevel);
  else
    bdk_window_unfullscreen (toplevel);
  
  bdk_window_set_keep_above (toplevel, priv->above_initially);

  bdk_window_set_keep_below (toplevel, priv->below_initially);

  /* No longer use the default settings */
  window->need_default_size = FALSE;
  window->need_default_position = FALSE;
  
  if (priv->reset_type_hint)
    {
      /* We should only reset the type hint when the application
       * used btk_window_set_type_hint() to change the hint.
       * Some applications use X directly to change the properties;
       * in that case, we shouldn't overwrite what they did.
       */
      bdk_window_set_type_hint (widget->window, priv->type_hint);
      priv->reset_type_hint = FALSE;
    }

  bdk_window_show (widget->window);

  if (window->frame)
    bdk_window_show (window->frame);

  if (!disable_startup_notification)
    {
      /* Do we have a custom startup-notification id? */
      if (priv->startup_id != NULL)
        {
          /* Make sure we have a "real" id */
          if (!startup_id_is_fake (priv->startup_id)) 
            bdk_notify_startup_complete_with_id (priv->startup_id);

          g_free (priv->startup_id);
          priv->startup_id = NULL;
        }
      else if (!sent_startup_notification)
        {
          sent_startup_notification = TRUE;
          bdk_notify_startup_complete ();
        }
    }

  /* if auto-mnemonics is enabled and mnemonics visible is not already set
   * (as in the case of popup menus), then hide mnemonics initially
   */
  g_object_get (btk_widget_get_settings (widget), "btk-auto-mnemonics",
                &auto_mnemonics, NULL);
  if (auto_mnemonics && !priv->mnemonics_visible_set)
    btk_window_set_mnemonics_visible (window, FALSE);
}

static gboolean
btk_window_map_event (BtkWidget   *widget,
                      BdkEventAny *event)
{
  if (!btk_widget_get_mapped (widget))
    {
      /* we should be be unmapped, but are getting a MapEvent, this may happen
       * to toplevel XWindows if mapping was intercepted by a window manager
       * and an unmap request occoured while the MapRequestEvent was still
       * being handled. we work around this situaiton here by re-requesting
       * the window being unmapped. more details can be found in:
       *   http://bugzilla.bunny.org/show_bug.cgi?id=316180
       */
      bdk_window_hide (widget->window);
    }
  return FALSE;
}

static void
btk_window_unmap (BtkWidget *widget)
{
  BtkWindow *window = BTK_WINDOW (widget);
  BtkWindowPrivate *priv = BTK_WINDOW_GET_PRIVATE (widget);
  BtkWindowGeometryInfo *info;    
  BdkWindowState state;

  btk_widget_set_mapped (widget, FALSE);
  if (window->frame)
    bdk_window_withdraw (window->frame);
  else 
    bdk_window_withdraw (widget->window);
  
  window->configure_request_count = 0;
  window->configure_notify_received = FALSE;

  /* on unmap, we reset the default positioning of the window,
   * so it's placed again, but we don't reset the default
   * size of the window, so it's remembered.
   */
  window->need_default_position = TRUE;

  info = btk_window_get_geometry_info (window, FALSE);
  if (info)
    {
      info->initial_pos_set = FALSE;
      info->position_constraints_changed = FALSE;
    }

  state = bdk_window_get_state (widget->window);
  window->iconify_initially = (state & BDK_WINDOW_STATE_ICONIFIED) != 0;
  window->maximize_initially = (state & BDK_WINDOW_STATE_MAXIMIZED) != 0;
  window->stick_initially = (state & BDK_WINDOW_STATE_STICKY) != 0;
  priv->above_initially = (state & BDK_WINDOW_STATE_ABOVE) != 0;
  priv->below_initially = (state & BDK_WINDOW_STATE_BELOW) != 0;
}

static void
btk_window_realize (BtkWidget *widget)
{
  BtkWindow *window;
  BdkWindow *parent_window;
  BdkWindowAttr attributes;
  gint attributes_mask;
  BtkWindowPrivate *priv;
  
  window = BTK_WINDOW (widget);
  priv = BTK_WINDOW_GET_PRIVATE (window);

  /* ensure widget tree is properly size allocated */
  if (widget->allocation.x == -1 &&
      widget->allocation.y == -1 &&
      widget->allocation.width == 1 &&
      widget->allocation.height == 1)
    {
      BtkRequisition requisition;
      BtkAllocation allocation = { 0, 0, 200, 200 };

      btk_widget_size_request (widget, &requisition);
      if (requisition.width || requisition.height)
	{
	  /* non-empty window */
	  allocation.width = requisition.width;
	  allocation.height = requisition.height;
	}
      btk_widget_size_allocate (widget, &allocation);
      
      _btk_container_queue_resize (BTK_CONTAINER (widget));

      g_return_if_fail (!btk_widget_get_realized (widget));
    }
  
  btk_widget_set_realized (widget, TRUE);
  
  switch (window->type)
    {
    case BTK_WINDOW_TOPLEVEL:
      attributes.window_type = BDK_WINDOW_TOPLEVEL;
      break;
    case BTK_WINDOW_POPUP:
      attributes.window_type = BDK_WINDOW_TEMP;
      break;
    default:
      g_warning (G_STRLOC": Unknown window type %d!", window->type);
      break;
    }
   
  attributes.title = window->title;
  attributes.wmclass_name = window->wmclass_name;
  attributes.wmclass_class = window->wmclass_class;
  attributes.wclass = BDK_INPUT_OUTPUT;
  attributes.visual = btk_widget_get_visual (widget);
  attributes.colormap = btk_widget_get_colormap (widget);

  if (window->has_frame)
    {
      attributes.width = widget->allocation.width + window->frame_left + window->frame_right;
      attributes.height = widget->allocation.height + window->frame_top + window->frame_bottom;
      attributes.event_mask = (BDK_EXPOSURE_MASK |
			       BDK_KEY_PRESS_MASK |
			       BDK_ENTER_NOTIFY_MASK |
			       BDK_LEAVE_NOTIFY_MASK |
			       BDK_FOCUS_CHANGE_MASK |
			       BDK_STRUCTURE_MASK |
			       BDK_BUTTON_MOTION_MASK |
			       BDK_POINTER_MOTION_HINT_MASK |
			       BDK_BUTTON_PRESS_MASK |
			       BDK_BUTTON_RELEASE_MASK);
      
      attributes_mask = BDK_WA_VISUAL | BDK_WA_COLORMAP;
      
      window->frame = bdk_window_new (btk_widget_get_root_window (widget),
				      &attributes, attributes_mask);
						 
      if (priv->opacity_set)
	bdk_window_set_opacity (window->frame, priv->opacity);

      bdk_window_set_user_data (window->frame, widget);
      
      attributes.window_type = BDK_WINDOW_CHILD;
      attributes.x = window->frame_left;
      attributes.y = window->frame_top;
    
      attributes_mask = BDK_WA_X | BDK_WA_Y;

      parent_window = window->frame;

      g_signal_connect (window,
			"event",
			G_CALLBACK (btk_window_event),
			NULL);
    }
  else
    {
      attributes_mask = 0;
      parent_window = btk_widget_get_root_window (widget);
    }
  
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_EXPOSURE_MASK |
			    BDK_KEY_PRESS_MASK |
			    BDK_KEY_RELEASE_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK |
			    BDK_FOCUS_CHANGE_MASK |
			    BDK_STRUCTURE_MASK);
  attributes.type_hint = priv->type_hint;

  attributes_mask |= BDK_WA_VISUAL | BDK_WA_COLORMAP | BDK_WA_TYPE_HINT;
  attributes_mask |= (window->title ? BDK_WA_TITLE : 0);
  attributes_mask |= (window->wmclass_name ? BDK_WA_WMCLASS : 0);
  
  widget->window = bdk_window_new (parent_window, &attributes, attributes_mask);

  if (!window->has_frame && priv->opacity_set)
    bdk_window_set_opacity (widget->window, priv->opacity);

  bdk_window_enable_synchronized_configure (widget->window);
    
  bdk_window_set_user_data (widget->window, window);
      
  widget->style = btk_style_attach (widget->style, widget->window);
  btk_style_set_background (widget->style, widget->window, BTK_STATE_NORMAL);
  if (window->frame)
    btk_style_set_background (widget->style, window->frame, BTK_STATE_NORMAL);

  /* This is a bad hack to set the window background. */
  btk_window_paint (widget, NULL);
  
  if (window->transient_parent &&
      btk_widget_get_realized (BTK_WIDGET (window->transient_parent)))
    bdk_window_set_transient_for (widget->window,
				  BTK_WIDGET (window->transient_parent)->window);

  if (window->wm_role)
    bdk_window_set_role (widget->window, window->wm_role);
  
  if (!window->decorated)
    bdk_window_set_decorations (widget->window, 0);

  if (!priv->deletable)
    bdk_window_set_functions (widget->window, BDK_FUNC_ALL | BDK_FUNC_CLOSE);

  if (btk_window_get_skip_pager_hint (window))
    bdk_window_set_skip_pager_hint (widget->window, TRUE);

  if (btk_window_get_skip_taskbar_hint (window))
    bdk_window_set_skip_taskbar_hint (widget->window, TRUE);

  if (btk_window_get_accept_focus (window))
    bdk_window_set_accept_focus (widget->window, TRUE);
  else
    bdk_window_set_accept_focus (widget->window, FALSE);

  if (btk_window_get_focus_on_map (window))
    bdk_window_set_focus_on_map (widget->window, TRUE);
  else
    bdk_window_set_focus_on_map (widget->window, FALSE);
  
  if (window->modal)
    bdk_window_set_modal_hint (widget->window, TRUE);
  else
    bdk_window_set_modal_hint (widget->window, FALSE);
    
  if (priv->startup_id)
    {
#ifdef BDK_WINDOWING_X11
      guint32 timestamp = extract_time_from_startup_id (priv->startup_id);
      if (timestamp != BDK_CURRENT_TIME)
        bdk_x11_window_set_user_time (widget->window, timestamp);
#endif
      if (!startup_id_is_fake (priv->startup_id)) 
        bdk_window_set_startup_id (widget->window, priv->startup_id);
    }

  /* Icons */
  btk_window_realize_icon (window);
}

static void
btk_window_unrealize (BtkWidget *widget)
{
  BtkWindow *window;
  BtkWindowGeometryInfo *info;

  window = BTK_WINDOW (widget);

  /* On unrealize, we reset the size of the window such
   * that we will re-apply the default sizing stuff
   * next time we show the window.
   *
   * Default positioning is reset on unmap, instead of unrealize.
   */
  window->need_default_size = TRUE;
  info = btk_window_get_geometry_info (window, FALSE);
  if (info)
    {
      info->resize_width = -1;
      info->resize_height = -1;
      info->last.configure_request.x = 0;
      info->last.configure_request.y = 0;
      info->last.configure_request.width = -1;
      info->last.configure_request.height = -1;
      /* be sure we reset geom hints on re-realize */
      info->last.flags = 0;
    }
  
  if (window->frame)
    {
      bdk_window_set_user_data (window->frame, NULL);
      bdk_window_destroy (window->frame);
      window->frame = NULL;
    }

  /* Icons */
  btk_window_unrealize_icon (window);

  BTK_WIDGET_CLASS (btk_window_parent_class)->unrealize (widget);
}

static void
btk_window_size_request (BtkWidget      *widget,
			 BtkRequisition *requisition)
{
  BtkWindow *window;
  BtkBin *bin;

  window = BTK_WINDOW (widget);
  bin = BTK_BIN (window);
  
  requisition->width = BTK_CONTAINER (window)->border_width * 2;
  requisition->height = BTK_CONTAINER (window)->border_width * 2;

  if (bin->child && btk_widget_get_visible (bin->child))
    {
      BtkRequisition child_requisition;
      
      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
}

static void
btk_window_size_allocate (BtkWidget     *widget,
			  BtkAllocation *allocation)
{
  BtkWindow *window;
  BtkAllocation child_allocation;

  window = BTK_WINDOW (widget);
  widget->allocation = *allocation;

  if (window->bin.child && btk_widget_get_visible (window->bin.child))
    {
      child_allocation.x = BTK_CONTAINER (window)->border_width;
      child_allocation.y = BTK_CONTAINER (window)->border_width;
      child_allocation.width =
	MAX (1, (gint)allocation->width - child_allocation.x * 2);
      child_allocation.height =
	MAX (1, (gint)allocation->height - child_allocation.y * 2);

      btk_widget_size_allocate (window->bin.child, &child_allocation);
    }

  if (btk_widget_get_realized (widget) && window->frame)
    {
      bdk_window_resize (window->frame,
			 allocation->width + window->frame_left + window->frame_right,
			 allocation->height + window->frame_top + window->frame_bottom);
    }
}

static gint
btk_window_event (BtkWidget *widget, BdkEvent *event)
{
  BtkWindow *window;
  gboolean return_val;

  window = BTK_WINDOW (widget);

  if (window->frame && (event->any.window == window->frame))
    {
      if ((event->type != BDK_KEY_PRESS) &&
	  (event->type != BDK_KEY_RELEASE) &&
	  (event->type != BDK_FOCUS_CHANGE))
	{
	  g_signal_stop_emission_by_name (widget, "event");
	  return_val = FALSE;
	  g_signal_emit (widget, window_signals[FRAME_EVENT], 0, event, &return_val);
	  return TRUE;
	}
      else
	{
	  g_object_unref (event->any.window);
	  event->any.window = g_object_ref (widget->window);
	}
    }

  return FALSE;
}

static gboolean
btk_window_frame_event (BtkWindow *window, BdkEvent *event)
{
  BdkEventConfigure *configure_event;
  BdkRectangle rect;

  switch (event->type)
    {
    case BDK_CONFIGURE:
      configure_event = (BdkEventConfigure *)event;
      
      /* Invalidate the decorations */
      rect.x = 0;
      rect.y = 0;
      rect.width = configure_event->width;
      rect.height = configure_event->height;
      
      bdk_window_invalidate_rect (window->frame, &rect, FALSE);

      /* Pass on the (modified) configure event */
      configure_event->width -= window->frame_left + window->frame_right;
      configure_event->height -= window->frame_top + window->frame_bottom;
      return btk_window_configure_event (BTK_WIDGET (window), configure_event);
      break;
    default:
      break;
    }
  return FALSE;
}

static gint
btk_window_configure_event (BtkWidget         *widget,
			    BdkEventConfigure *event)
{
  BtkWindow *window = BTK_WINDOW (widget);
  gboolean expected_reply = window->configure_request_count > 0;

  /* window->configure_request_count incremented for each 
   * configure request, and decremented to a min of 0 for
   * each configure notify.
   *
   * All it means is that we know we will get at least
   * window->configure_request_count more configure notifies.
   * We could get more configure notifies than that; some
   * of the configure notifies we get may be unrelated to
   * the configure requests. But we will get at least
   * window->configure_request_count notifies.
   */

  if (window->configure_request_count > 0)
    {
      window->configure_request_count -= 1;
      bdk_window_thaw_toplevel_updates_libbtk_only (widget->window);
    }
  
  /* As an optimization, we avoid a resize when possible.
   *
   * The only times we can avoid a resize are:
   *   - we know only the position changed, not the size
   *   - we know we have made more requests and so will get more
   *     notifies and can wait to resize when we get them
   */
  
  if (!expected_reply &&
      (widget->allocation.width == event->width &&
       widget->allocation.height == event->height))
    {
      bdk_window_configure_finished (widget->window);
      return TRUE;
    }

  /*
   * If we do need to resize, we do that by:
   *   - filling in widget->allocation with the new size
   *   - setting configure_notify_received to TRUE
   *     for use in btk_window_move_resize()
   *   - queueing a resize, leading to invocation of
   *     btk_window_move_resize() in an idle handler
   *
   */
  
  window->configure_notify_received = TRUE;
  
  widget->allocation.width = event->width;
  widget->allocation.height = event->height;
  
  _btk_container_queue_resize (BTK_CONTAINER (widget));
  
  return TRUE;
}

/* the accel_key and accel_mods fields of the key have to be setup
 * upon calling this function. it'll then return whether that key
 * is at all used as accelerator, and if so will OR in the
 * accel_flags member of the key.
 */
gboolean
_btk_window_query_nonaccels (BtkWindow      *window,
			     guint           accel_key,
			     BdkModifierType accel_mods)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  /* movement keys are considered locked accels */
  if (!accel_mods)
    {
      static const guint bindings[] = {
	BDK_space, BDK_KP_Space, BDK_Return, BDK_ISO_Enter, BDK_KP_Enter, BDK_Up, BDK_KP_Up, BDK_Down, BDK_KP_Down,
	BDK_Left, BDK_KP_Left, BDK_Right, BDK_KP_Right, BDK_Tab, BDK_KP_Tab, BDK_ISO_Left_Tab,
      };
      guint i;
      
      for (i = 0; i < G_N_ELEMENTS (bindings); i++)
	if (bindings[i] == accel_key)
	  return TRUE;
    }

  /* mnemonics are considered locked accels */
  if (accel_mods == window->mnemonic_modifier)
    {
      BtkMnemonicHash *mnemonic_hash = btk_window_get_mnemonic_hash (window, FALSE);
      if (mnemonic_hash && _btk_mnemonic_hash_lookup (mnemonic_hash, accel_key))
	return TRUE;
    }

  return FALSE;
}

/**
 * btk_window_propagate_key_event:
 * @window:  a #BtkWindow
 * @event:   a #BdkEventKey
 *
 * Propagate a key press or release event to the focus widget and
 * up the focus container chain until a widget handles @event.
 * This is normally called by the default ::key_press_event and
 * ::key_release_event handlers for toplevel windows,
 * however in some cases it may be useful to call this directly when
 * overriding the standard key handling for a toplevel window.
 *
 * Return value: %TRUE if a widget in the focus chain handled the event.
 *
 * Since: 2.4
 */
gboolean
btk_window_propagate_key_event (BtkWindow        *window,
                                BdkEventKey      *event)
{
  gboolean handled = FALSE;
  BtkWidget *widget, *focus;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  widget = BTK_WIDGET (window);
  focus = window->focus_widget;
  if (focus)
    g_object_ref (focus);
  
  while (!handled &&
         focus && focus != widget &&
         btk_widget_get_toplevel (focus) == widget)
    {
      BtkWidget *parent;
      
      if (btk_widget_is_sensitive (focus))
        handled = btk_widget_event (focus, (BdkEvent*) event);
      
      parent = focus->parent;
      if (parent)
        g_object_ref (parent);
      
      g_object_unref (focus);
      
      focus = parent;
    }
  
  if (focus)
    g_object_unref (focus);

  return handled;
}

static gint
btk_window_key_press_event (BtkWidget   *widget,
			    BdkEventKey *event)
{
  BtkWindow *window = BTK_WINDOW (widget);
  gboolean handled = FALSE;

  /* handle mnemonics and accelerators */
  if (!handled)
    handled = btk_window_activate_key (window, event);

  /* handle focus widget key events */
  if (!handled)
    handled = btk_window_propagate_key_event (window, event);

  /* Chain up, invokes binding set */
  if (!handled)
    handled = BTK_WIDGET_CLASS (btk_window_parent_class)->key_press_event (widget, event);

  return handled;
}

static gint
btk_window_key_release_event (BtkWidget   *widget,
			      BdkEventKey *event)
{
  BtkWindow *window = BTK_WINDOW (widget);
  gboolean handled = FALSE;

  /* handle focus widget key events */
  if (!handled)
    handled = btk_window_propagate_key_event (window, event);

  /* Chain up, invokes binding set */
  if (!handled)
    handled = BTK_WIDGET_CLASS (btk_window_parent_class)->key_release_event (widget, event);

  return handled;
}

static void
btk_window_real_activate_default (BtkWindow *window)
{
  btk_window_activate_default (window);
}

static void
btk_window_real_activate_focus (BtkWindow *window)
{
  btk_window_activate_focus (window);
}

static void
btk_window_move_focus (BtkWindow       *window,
                       BtkDirectionType dir)
{
  btk_widget_child_focus (BTK_WIDGET (window), dir);
  
  if (!BTK_CONTAINER (window)->focus_child)
    btk_window_set_focus (window, NULL);
}

static gint
btk_window_enter_notify_event (BtkWidget        *widget,
			       BdkEventCrossing *event)
{
  return FALSE;
}

static gint
btk_window_leave_notify_event (BtkWidget        *widget,
			       BdkEventCrossing *event)
{
  return FALSE;
}

static void
do_focus_change (BtkWidget *widget,
		 gboolean   in)
{
  BdkEvent *fevent = bdk_event_new (BDK_FOCUS_CHANGE);
  
  fevent->focus_change.type = BDK_FOCUS_CHANGE;
  fevent->focus_change.window = widget->window;
  fevent->focus_change.in = in;
  if (widget->window)
    g_object_ref (widget->window);

  btk_widget_send_focus_change (widget, fevent);

  bdk_event_free (fevent);
}

static gint
btk_window_focus_in_event (BtkWidget     *widget,
			   BdkEventFocus *event)
{
  BtkWindow *window = BTK_WINDOW (widget);

  /* It appears spurious focus in events can occur when
   *  the window is hidden. So we'll just check to see if
   *  the window is visible before actually handling the
   *  event
   */
  if (btk_widget_get_visible (widget))
    {
      _btk_window_set_has_toplevel_focus (window, TRUE);
      _btk_window_set_is_active (window, TRUE);
    }
      
  return FALSE;
}

static gint
btk_window_focus_out_event (BtkWidget     *widget,
			    BdkEventFocus *event)
{
  BtkWindow *window = BTK_WINDOW (widget);
  gboolean auto_mnemonics;

  _btk_window_set_has_toplevel_focus (window, FALSE);
  _btk_window_set_is_active (window, FALSE);

  /* set the mnemonic-visible property to false */
  g_object_get (btk_widget_get_settings (widget),
                "btk-auto-mnemonics", &auto_mnemonics, NULL);
  if (auto_mnemonics)
    btk_window_set_mnemonics_visible (window, FALSE);

  return FALSE;
}

static BdkAtom atom_rcfiles = BDK_NONE;
static BdkAtom atom_iconthemes = BDK_NONE;

static void
send_client_message_to_embedded_windows (BtkWidget *widget,
					 BdkAtom    message_type)
{
  GList *embedded_windows;

  embedded_windows = g_object_get_qdata (G_OBJECT (widget), quark_btk_embedded);
  if (embedded_windows)
    {
      BdkEvent *send_event = bdk_event_new (BDK_CLIENT_EVENT);
      int i;
      
      for (i = 0; i < 5; i++)
	send_event->client.data.l[i] = 0;
      send_event->client.data_format = 32;
      send_event->client.message_type = message_type;
      
      while (embedded_windows)
	{
	  BdkNativeWindow xid = BDK_GPOINTER_TO_NATIVE_WINDOW(embedded_windows->data);
	  bdk_event_send_client_message_for_display (btk_widget_get_display (widget), send_event, xid);
	  embedded_windows = embedded_windows->next;
	}

      bdk_event_free (send_event);
    }
}

static gint
btk_window_client_event (BtkWidget	*widget,
			 BdkEventClient	*event)
{
  if (!atom_rcfiles)
    {
      atom_rcfiles = bdk_atom_intern_static_string ("_BTK_READ_RCFILES");
      atom_iconthemes = bdk_atom_intern_static_string ("_BTK_LOAD_ICONTHEMES");
    }

  if (event->message_type == atom_rcfiles) 
    {
      send_client_message_to_embedded_windows (widget, atom_rcfiles);
      btk_rc_reparse_all_for_settings (btk_widget_get_settings (widget), FALSE);
    }

  if (event->message_type == atom_iconthemes) 
    {
      send_client_message_to_embedded_windows (widget, atom_iconthemes);
      _btk_icon_theme_check_reload (btk_widget_get_display (widget));    
    }

  return FALSE;
}

static void
btk_window_check_resize (BtkContainer *container)
{
  if (btk_widget_get_visible (BTK_WIDGET (container)))
    btk_window_move_resize (BTK_WINDOW (container));
}

static gboolean
btk_window_focus (BtkWidget        *widget,
		  BtkDirectionType  direction)
{
  BtkBin *bin;
  BtkWindow *window;
  BtkContainer *container;
  BtkWidget *old_focus_child;
  BtkWidget *parent;

  container = BTK_CONTAINER (widget);
  window = BTK_WINDOW (widget);
  bin = BTK_BIN (widget);

  old_focus_child = container->focus_child;
  
  /* We need a special implementation here to deal properly with wrapping
   * around in the tab chain without the danger of going into an
   * infinite loop.
   */
  if (old_focus_child)
    {
      if (btk_widget_child_focus (old_focus_child, direction))
	return TRUE;
    }

  if (window->focus_widget)
    {
      if (direction == BTK_DIR_LEFT ||
	  direction == BTK_DIR_RIGHT ||
	  direction == BTK_DIR_UP ||
	  direction == BTK_DIR_DOWN)
	{
	  return FALSE;
	}
      
      /* Wrapped off the end, clear the focus setting for the toplpevel */
      parent = window->focus_widget->parent;
      while (parent)
	{
	  btk_container_set_focus_child (BTK_CONTAINER (parent), NULL);
	  parent = BTK_WIDGET (parent)->parent;
	}
      
      btk_window_set_focus (BTK_WINDOW (container), NULL);
    }

  /* Now try to focus the first widget in the window */
  if (bin->child)
    {
      if (btk_widget_child_focus (bin->child, direction))
        return TRUE;
    }

  return FALSE;
}

static void
btk_window_real_set_focus (BtkWindow *window,
			   BtkWidget *focus)
{
  BtkWidget *old_focus = window->focus_widget;
  gboolean had_default = FALSE;
  gboolean focus_had_default = FALSE;
  gboolean old_focus_had_default = FALSE;

  if (old_focus)
    {
      g_object_ref (old_focus);
      g_object_freeze_notify (G_OBJECT (old_focus));
      old_focus_had_default = btk_widget_has_default (old_focus);
    }
  if (focus)
    {
      g_object_ref (focus);
      g_object_freeze_notify (G_OBJECT (focus));
      focus_had_default = btk_widget_has_default (focus);
    }
  
  if (window->default_widget)
    had_default = btk_widget_has_default (window->default_widget);
  
  if (window->focus_widget)
    {
      if (btk_widget_get_receives_default (window->focus_widget) &&
	  (window->focus_widget != window->default_widget))
        {
          _btk_widget_set_has_default (window->focus_widget, FALSE);
	  btk_widget_queue_draw (window->focus_widget);
	  
	  if (window->default_widget)
            _btk_widget_set_has_default (window->default_widget, TRUE);
	}

      window->focus_widget = NULL;

      if (window->has_focus)
	do_focus_change (old_focus, FALSE);

      g_object_notify (G_OBJECT (old_focus), "is-focus");
    }

  /* The above notifications may have set a new focus widget,
   * if so, we don't want to override it.
   */
  if (focus && !window->focus_widget)
    {
      window->focus_widget = focus;
  
      if (btk_widget_get_receives_default (window->focus_widget) &&
	  (window->focus_widget != window->default_widget))
	{
	  if (btk_widget_get_can_default (window->focus_widget))
            _btk_widget_set_has_default (window->focus_widget, TRUE);

	  if (window->default_widget)
            _btk_widget_set_has_default (window->default_widget, FALSE);
	}

      if (window->has_focus)
	do_focus_change (window->focus_widget, TRUE);

      g_object_notify (G_OBJECT (window->focus_widget), "is-focus");
    }

  /* If the default widget changed, a redraw will have been queued
   * on the old and new default widgets by btk_window_set_default(), so
   * we only have to worry about the case where it didn't change.
   * We'll sometimes queue a draw twice on the new widget but that
   * is harmless.
   */
  if (window->default_widget &&
      (had_default != btk_widget_has_default (window->default_widget)))
    btk_widget_queue_draw (window->default_widget);
  
  if (old_focus)
    {
      if (old_focus_had_default != btk_widget_has_default (old_focus))
	btk_widget_queue_draw (old_focus);
	
      g_object_thaw_notify (G_OBJECT (old_focus));
      g_object_unref (old_focus);
    }
  if (focus)
    {
      if (focus_had_default != btk_widget_has_default (focus))
	btk_widget_queue_draw (focus);

      g_object_thaw_notify (G_OBJECT (focus));
      g_object_unref (focus);
    }
}

/**
 * _btk_window_unset_focus_and_default:
 * @window: a #BtkWindow
 * @widget: a widget inside of @window
 * 
 * Checks whether the focus and default widgets of @window are
 * @widget or a descendent of @widget, and if so, unset them.
 **/
void
_btk_window_unset_focus_and_default (BtkWindow *window,
				     BtkWidget *widget)

{
  BtkWidget *child;

  g_object_ref (window);
  g_object_ref (widget);
      
  if (BTK_CONTAINER (widget->parent)->focus_child == widget)
    {
      child = window->focus_widget;
      
      while (child && child != widget)
	child = child->parent;
  
      if (child == widget)
	btk_window_set_focus (BTK_WINDOW (window), NULL);
    }
      
  child = window->default_widget;
      
  while (child && child != widget)
    child = child->parent;
      
  if (child == widget)
    btk_window_set_default (window, NULL);
  
  g_object_unref (widget);
  g_object_unref (window);
}

/*********************************
 * Functions related to resizing *
 *********************************/

/* This function doesn't constrain to geometry hints */
static void 
btk_window_compute_configure_request_size (BtkWindow *window,
                                           guint     *width,
                                           guint     *height)
{
  BtkRequisition requisition;
  BtkWindowGeometryInfo *info;
  BtkWidget *widget;

  /* Preconditions:
   *  - we've done a size request
   */
  
  widget = BTK_WIDGET (window);

  info = btk_window_get_geometry_info (window, FALSE);
  
  if (window->need_default_size)
    {
      btk_widget_get_child_requisition (widget, &requisition);

      /* Default to requisition */
      *width = requisition.width;
      *height = requisition.height;

      /* If window is empty so requests 0, default to random nonzero size */
       if (*width == 0 && *height == 0)
         {
           *width = 200;
           *height = 200;
         }

       /* Override requisition with default size */

       if (info)
         {
	   gint base_width = 0;
	   gint base_height = 0;
	   gint min_width = 0;
	   gint min_height = 0;
	   gint width_inc = 1;
	   gint height_inc = 1;
	   
	   if (info->default_is_geometry &&
	       (info->default_width > 0 || info->default_height > 0))
	     {
	       BdkGeometry geometry;
	       guint flags;
	       
	       btk_window_compute_hints (window, &geometry, &flags);

	       if (flags & BDK_HINT_BASE_SIZE)
		 {
		   base_width = geometry.base_width;
		   base_height = geometry.base_height;
		 }
	       if (flags & BDK_HINT_MIN_SIZE)
		 {
		   min_width = geometry.min_width;
		   min_height = geometry.min_height;
		 }
	       if (flags & BDK_HINT_RESIZE_INC)
		 {
		   width_inc = geometry.width_inc;
		   height_inc = geometry.height_inc;
		 }
	     }
	     
	   if (info->default_width > 0)
	     *width = MAX (info->default_width * width_inc + base_width, min_width);
	   
	   if (info->default_height > 0)
	     *height = MAX (info->default_height * height_inc + base_height, min_height);
         }
    }
  else
    {
      /* Default to keeping current size */
      *width = widget->allocation.width;
      *height = widget->allocation.height;
    }

  /* Override any size with btk_window_resize() values */
  if (info)
    {
      if (info->resize_width > 0)
        *width = info->resize_width;

      if (info->resize_height > 0)
        *height = info->resize_height;
    }

  /* Don't ever request zero width or height, its not supported by
     bdk. The size allocation code will round it to 1 anyway but if
     we do it then the value returned from this function will is
     not comparable to the size allocation read from the BtkWindow. */
  *width = MAX (*width, 1);
  *height = MAX (*height, 1);
}

static BtkWindowPosition
get_effective_position (BtkWindow *window)
{
  BtkWindowPosition pos = window->position;

  if (pos == BTK_WIN_POS_CENTER_ON_PARENT &&
      (window->transient_parent == NULL ||
       !btk_widget_get_mapped (BTK_WIDGET (window->transient_parent))))
    pos = BTK_WIN_POS_NONE;

  return pos;
}

static int
get_center_monitor_of_window (BtkWindow *window)
{
  /* We could try to sort out the relative positions of the monitors and
   * stuff, or we could just be losers and assume you have a row
   * or column of monitors.
   */
  return bdk_screen_get_n_monitors (btk_window_check_screen (window)) / 2;
}

static int
get_monitor_containing_pointer (BtkWindow *window)
{
  gint px, py;
  gint monitor_num;
  BdkScreen *window_screen;
  BdkScreen *pointer_screen;

  window_screen = btk_window_check_screen (window);
  bdk_display_get_pointer (bdk_screen_get_display (window_screen),
                           &pointer_screen,
                           &px, &py, NULL);

  if (pointer_screen == window_screen)
    monitor_num = bdk_screen_get_monitor_at_point (pointer_screen, px, py);
  else
    monitor_num = -1;

  return monitor_num;
}

static void
center_window_on_monitor (BtkWindow *window,
                          gint       w,
                          gint       h,
                          gint      *x,
                          gint      *y)
{
  BdkRectangle monitor;
  int monitor_num;

  monitor_num = get_monitor_containing_pointer (window);
  
  if (monitor_num == -1)
    monitor_num = get_center_monitor_of_window (window);

  bdk_screen_get_monitor_geometry (btk_window_check_screen (window),
				   monitor_num, &monitor);
  
  *x = (monitor.width - w) / 2 + monitor.x;
  *y = (monitor.height - h) / 2 + monitor.y;

  /* Be sure we aren't off the monitor, ignoring _NET_WM_STRUT
   * and WM decorations.
   */
  if (*x < monitor.x)
    *x = monitor.x;
  if (*y < monitor.y)
    *y = monitor.y;
}

static void
clamp (gint *base,
       gint  extent,
       gint  clamp_base,
       gint  clamp_extent)
{
  if (extent > clamp_extent)
    /* Center */
    *base = clamp_base + clamp_extent/2 - extent/2;
  else if (*base < clamp_base)
    *base = clamp_base;
  else if (*base + extent > clamp_base + clamp_extent)
    *base = clamp_base + clamp_extent - extent;
}

static void
clamp_window_to_rectangle (gint               *x,
                           gint               *y,
                           gint                w,
                           gint                h,
                           const BdkRectangle *rect)
{
#ifdef DEBUGGING_OUTPUT
  g_print ("%s: %+d%+d %dx%d: %+d%+d: %dx%d", G_STRFUNC, rect->x, rect->y, rect->width, rect->height, *x, *y, w, h);
#endif

  /* If it is too large, center it. If it fits on the monitor but is
   * partially outside, move it to the closest edge. Do this
   * separately in x and y directions.
   */
  clamp (x, w, rect->x, rect->width);
  clamp (y, h, rect->y, rect->height);
#ifdef DEBUGGING_OUTPUT
  g_print (" ==> %+d%+d: %dx%d\n", *x, *y, w, h);
#endif
}


static void
btk_window_compute_configure_request (BtkWindow    *window,
                                      BdkRectangle *request,
                                      BdkGeometry  *geometry,
                                      guint        *flags)
{
  BdkGeometry new_geometry;
  guint new_flags;
  int w, h;
  BtkWidget *widget;
  BtkWindowPosition pos;
  BtkWidget *parent_widget;
  BtkWindowGeometryInfo *info;
  BdkScreen *screen;
  int x, y;
  
  widget = BTK_WIDGET (window);

  screen = btk_window_check_screen (window);
  
  btk_widget_size_request (widget, NULL);
  btk_window_compute_configure_request_size (window, (guint *)&w, (guint *)&h);
  
  btk_window_compute_hints (window, &new_geometry, &new_flags);
  btk_window_constrain_size (window,
                             &new_geometry, new_flags,
                             w, h,
                             &w, &h);

  parent_widget = (BtkWidget*) window->transient_parent;
  
  pos = get_effective_position (window);
  info = btk_window_get_geometry_info (window, FALSE);
  
  /* by default, don't change position requested */
  if (info)
    {
      x = info->last.configure_request.x;
      y = info->last.configure_request.y;
    }
  else
    {
      x = 0;
      y = 0;
    }


  if (window->need_default_position)
    {

      /* FIXME this all interrelates with window gravity.
       * For most of them I think we want to set GRAVITY_CENTER.
       *
       * Not sure how to go about that.
       */
      
      switch (pos)
        {
          /* here we are only handling CENTER_ALWAYS
           * as it relates to default positioning,
           * where it's equivalent to simply CENTER
           */
        case BTK_WIN_POS_CENTER_ALWAYS:
        case BTK_WIN_POS_CENTER:
          center_window_on_monitor (window, w, h, &x, &y);
          break;
      
        case BTK_WIN_POS_CENTER_ON_PARENT:
          {
            gint monitor_num;
            BdkRectangle monitor;
            gint ox, oy;
            
            g_assert (btk_widget_get_mapped (parent_widget)); /* established earlier */

            if (parent_widget->window != NULL)
              monitor_num = bdk_screen_get_monitor_at_window (screen,
                                                              parent_widget->window);
            else
              monitor_num = -1;
            
            bdk_window_get_origin (parent_widget->window,
                                   &ox, &oy);
            
            x = ox + (parent_widget->allocation.width - w) / 2;
            y = oy + (parent_widget->allocation.height - h) / 2;
            
            /* Clamp onto current monitor, ignoring _NET_WM_STRUT and
             * WM decorations. If parent wasn't on a monitor, just
             * give up.
             */
            if (monitor_num >= 0)
              {
                bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
                clamp_window_to_rectangle (&x, &y, w, h, &monitor);
              }
          }
          break;

        case BTK_WIN_POS_MOUSE:
          {
            gint screen_width = bdk_screen_get_width (screen);
            gint screen_height = bdk_screen_get_height (screen);
	    gint monitor_num;
	    BdkRectangle monitor;
            BdkScreen *pointer_screen;
            gint px, py;
            
            bdk_display_get_pointer (bdk_screen_get_display (screen),
                                     &pointer_screen,
                                     &px, &py, NULL);

            if (pointer_screen == screen)
              monitor_num = bdk_screen_get_monitor_at_point (screen, px, py);
            else
              monitor_num = -1;
            
            x = px - w / 2;
            y = py - h / 2;
            x = CLAMP (x, 0, screen_width - w);
            y = CLAMP (y, 0, screen_height - h);

            /* Clamp onto current monitor, ignoring _NET_WM_STRUT and
             * WM decorations. Don't try to figure out what's going
             * on if the mouse wasn't inside a monitor.
             */
            if (monitor_num >= 0)
              {
                bdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
                clamp_window_to_rectangle (&x, &y, w, h, &monitor);
              }
          }
          break;

        default:
          break;
        }
    } /* if (window->need_default_position) */

  if (window->need_default_position && info &&
      info->initial_pos_set)
    {
      x = info->initial_x;
      y = info->initial_y;
      btk_window_constrain_position (window, w, h, &x, &y);
    }
  
  request->x = x;
  request->y = y;
  request->width = w;
  request->height = h;

  if (geometry)
    *geometry = new_geometry;
  if (flags)
    *flags = new_flags;
}

static void
btk_window_constrain_position (BtkWindow    *window,
                               gint          new_width,
                               gint          new_height,
                               gint         *x,
                               gint         *y)
{
  /* See long comments in btk_window_move_resize()
   * on when it's safe to call this function.
   */
  if (window->position == BTK_WIN_POS_CENTER_ALWAYS)
    {
      gint center_x, center_y;

      center_window_on_monitor (window, new_width, new_height, &center_x, &center_y);
      
      *x = center_x;
      *y = center_y;
    }
}

static void
btk_window_move_resize (BtkWindow *window)
{
  /* Overview:
   *
   * First we determine whether any information has changed that would
   * cause us to revise our last configure request.  If we would send
   * a different configure request from last time, then
   * configure_request_size_changed = TRUE or
   * configure_request_pos_changed = TRUE. configure_request_size_changed
   * may be true due to new hints, a btk_window_resize(), or whatever.
   * configure_request_pos_changed may be true due to btk_window_set_position()
   * or btk_window_move().
   *
   * If the configure request has changed, we send off a new one.  To
   * ensure BTK+ invariants are maintained (resize queue does what it
   * should), we go ahead and size_allocate the requested size in this
   * function.
   *
   * If the configure request has not changed, we don't ever resend
   * it, because it could mean fighting the user or window manager.
   *
   * 
   *   To prepare the configure request, we come up with a base size/pos:
   *      - the one from btk_window_move()/btk_window_resize()
   *      - else default_width, default_height if we haven't ever
   *        been mapped
   *      - else the size request if we haven't ever been mapped,
   *        as a substitute default size
   *      - else the current size of the window, as received from
   *        configure notifies (i.e. the current allocation)
   *
   *   If BTK_WIN_POS_CENTER_ALWAYS is active, we constrain
   *   the position request to be centered.
   */
  BtkWidget *widget;
  BtkContainer *container;
  BtkWindowGeometryInfo *info;
  BdkGeometry new_geometry;
  guint new_flags;
  BdkRectangle new_request;
  gboolean configure_request_size_changed;
  gboolean configure_request_pos_changed;
  gboolean hints_changed; /* do we need to send these again */
  BtkWindowLastGeometryInfo saved_last_info;
  
  widget = BTK_WIDGET (window);
  container = BTK_CONTAINER (widget);
  info = btk_window_get_geometry_info (window, TRUE);
  
  configure_request_size_changed = FALSE;
  configure_request_pos_changed = FALSE;
  
  btk_window_compute_configure_request (window, &new_request,
                                        &new_geometry, &new_flags);  
  
  /* This check implies the invariant that we never set info->last
   * without setting the hints and sending off a configure request.
   *
   * If we change info->last without sending the request, we may
   * miss a request.
   */
  if (info->last.configure_request.x != new_request.x ||
      info->last.configure_request.y != new_request.y)
    configure_request_pos_changed = TRUE;

  if ((info->last.configure_request.width != new_request.width ||
       info->last.configure_request.height != new_request.height))
    configure_request_size_changed = TRUE;
  
  hints_changed = FALSE;
  
  if (!btk_window_compare_hints (&info->last.geometry, info->last.flags,
				 &new_geometry, new_flags))
    {
      hints_changed = TRUE;
    }
  
  /* Position Constraints
   * ====================
   * 
   * POS_CENTER_ALWAYS is conceptually a constraint rather than
   * a default. The other POS_ values are used only when the
   * window is shown, not after that.
   * 
   * However, we can't implement a position constraint as
   * "anytime the window size changes, center the window"
   * because this may well end up fighting the WM or user.  In
   * fact it gets in an infinite loop with at least one WM.
   *
   * Basically, applications are in no way in a position to
   * constrain the position of a window, with one exception:
   * override redirect windows. (Really the intended purpose
   * of CENTER_ALWAYS anyhow, I would think.)
   *
   * So the way we implement this "constraint" is to say that when WE
   * cause a move or resize, i.e. we make a configure request changing
   * window size, we recompute the CENTER_ALWAYS position to reflect
   * the new window size, and include it in our request.  Also, if we
   * just turned on CENTER_ALWAYS we snap to center with a new
   * request.  Otherwise, if we are just NOTIFIED of a move or resize
   * done by someone else e.g. the window manager, we do NOT send a
   * new configure request.
   *
   * For override redirect windows, this works fine; all window
   * sizes are from our configure requests. For managed windows,
   * it is at least semi-sane, though who knows what the
   * app author is thinking.
   */

  /* This condition should be kept in sync with the condition later on
   * that determines whether we send a configure request.  i.e. we
   * should do this position constraining anytime we were going to
   * send a configure request anyhow, plus when constraints have
   * changed.
   */
  if (configure_request_pos_changed ||
      configure_request_size_changed ||
      hints_changed ||
      info->position_constraints_changed)
    {
      /* We request the constrained position if:
       *  - we were changing position, and need to clamp
       *    the change to the constraint
       *  - we're changing the size anyway
       *  - set_position() was called to toggle CENTER_ALWAYS on
       */

      btk_window_constrain_position (window,
                                     new_request.width,
                                     new_request.height,
                                     &new_request.x,
                                     &new_request.y);
      
      /* Update whether we need to request a move */
      if (info->last.configure_request.x != new_request.x ||
          info->last.configure_request.y != new_request.y)
        configure_request_pos_changed = TRUE;
      else
        configure_request_pos_changed = FALSE;
    }

#if 0
  if (window->type == BTK_WINDOW_TOPLEVEL)
    {
      int notify_x, notify_y;

      /* this is the position from the last configure notify */
      bdk_window_get_position (widget->window, &notify_x, &notify_y);
    
      g_message ("--- %s ---\n"
		 "last  : %d,%d\t%d x %d\n"
		 "this  : %d,%d\t%d x %d\n"
		 "alloc : %d,%d\t%d x %d\n"
		 "req   :      \t%d x %d\n"
		 "resize:      \t%d x %d\n" 
		 "size_changed: %d pos_changed: %d hints_changed: %d\n"
		 "configure_notify_received: %d\n"
		 "configure_request_count: %d\n"
		 "position_constraints_changed: %d\n",
		 window->title ? window->title : "(no title)",
		 info->last.configure_request.x,
		 info->last.configure_request.y,
		 info->last.configure_request.width,
		 info->last.configure_request.height,
		 new_request.x,
		 new_request.y,
		 new_request.width,
		 new_request.height,
		 notify_x, notify_y,
		 widget->allocation.width,
		 widget->allocation.height,
		 widget->requisition.width,
		 widget->requisition.height,
		 info->resize_width,
		 info->resize_height,
		 configure_request_pos_changed,
		 configure_request_size_changed,
		 hints_changed,
		 window->configure_notify_received,
		 window->configure_request_count,
		 info->position_constraints_changed);
    }
#endif
  
  saved_last_info = info->last;
  info->last.geometry = new_geometry;
  info->last.flags = new_flags;
  info->last.configure_request = new_request;
  
  /* need to set PPosition so the WM will look at our position,
   * but we don't want to count PPosition coming and going as a hints
   * change for future iterations. So we saved info->last prior to
   * this.
   */
  
  /* Also, if the initial position was explicitly set, then we always
   * toggle on PPosition. This makes btk_window_move(window, 0, 0)
   * work.
   */

  /* Also, we toggle on PPosition if BTK_WIN_POS_ is in use and
   * this is an initial map
   */
  
  if ((configure_request_pos_changed ||
       info->initial_pos_set ||
       (window->need_default_position &&
        get_effective_position (window) != BTK_WIN_POS_NONE)) &&
      (new_flags & BDK_HINT_POS) == 0)
    {
      new_flags |= BDK_HINT_POS;
      hints_changed = TRUE;
    }
  
  /* Set hints if necessary
   */
  if (hints_changed)
    bdk_window_set_geometry_hints (widget->window,
				   &new_geometry,
				   new_flags);
  
  /* handle resizing/moving and widget tree allocation
   */
  if (window->configure_notify_received)
    { 
      BtkAllocation allocation;

      /* If we have received a configure event since
       * the last time in this function, we need to
       * accept our new size and size_allocate child widgets.
       * (see btk_window_configure_event() for more details).
       *
       * 1 or more configure notifies may have been received.
       * Also, configure_notify_received will only be TRUE
       * if all expected configure notifies have been received
       * (one per configure request), as an optimization.
       *
       */
      window->configure_notify_received = FALSE;

      /* btk_window_configure_event() filled in widget->allocation */
      allocation = widget->allocation;
      btk_widget_size_allocate (widget, &allocation);

      bdk_window_process_updates (widget->window, TRUE);
      
      bdk_window_configure_finished (widget->window);

      /* If the configure request changed, it means that
       * we either:
       *   1) coincidentally changed hints or widget properties
       *      impacting the configure request before getting
       *      a configure notify, or
       *   2) some broken widget is changing its size request
       *      during size allocation, resulting in
       *      a false appearance of changed configure request.
       *
       * For 1), we could just go ahead and ask for the
       * new size right now, but doing that for 2)
       * might well be fighting the user (and can even
       * trigger a loop). Since we really don't want to
       * do that, we requeue a resize in hopes that
       * by the time it gets handled, the child has seen
       * the light and is willing to go along with the
       * new size. (this happens for the zvt widget, since
       * the size_allocate() above will have stored the
       * requisition corresponding to the new size in the
       * zvt widget)
       *
       * This doesn't buy us anything for 1), but it shouldn't
       * hurt us too badly, since it is what would have
       * happened if we had gotten the configure event before
       * the new size had been set.
       */

      if (configure_request_size_changed ||
          configure_request_pos_changed)
        {
          /* Don't change the recorded last info after all, because we
           * haven't actually updated to the new info yet - we decided
           * to postpone our configure request until later.
           */
	  info->last = saved_last_info;
          
	  btk_widget_queue_resize_no_redraw (widget); /* migth recurse for BTK_RESIZE_IMMEDIATE */
	}

      return;			/* Bail out, we didn't really process the move/resize */
    }
  else if ((configure_request_size_changed || hints_changed) &&
	   (widget->allocation.width != new_request.width ||
	    widget->allocation.height != new_request.height))

    {
      /* We are in one of the following situations:
       * A. configure_request_size_changed
       *    our requisition has changed and we need a different window size,
       *    so we request it from the window manager.
       * B. !configure_request_size_changed && hints_changed
       *    the window manager rejects our size, but we have just changed the
       *    window manager hints, so there's a chance our request will
       *    be honoured this time, so we try again.
       *
       * However, if the new requisition is the same as the current allocation,
       * we don't request it again, since we won't get a ConfigureNotify back from
       * the window manager unless it decides to change our requisition. If
       * we don't get the ConfigureNotify back, the resize queue will never be run.
       */

      /* Now send the configure request */
      if (configure_request_pos_changed)
	{
	  if (window->frame)
	    {
	      bdk_window_move_resize (window->frame,
				      new_request.x - window->frame_left,
                                      new_request.y - window->frame_top,
				      new_request.width + window->frame_left + window->frame_right,
				      new_request.height + window->frame_top + window->frame_bottom);
	      bdk_window_resize (widget->window,
                                 new_request.width, new_request.height);
	    }
	  else
	    bdk_window_move_resize (widget->window,
				    new_request.x, new_request.y,
				    new_request.width, new_request.height);
	}
      else  /* only size changed */
	{
	  if (window->frame)
	    bdk_window_resize (window->frame,
			       new_request.width + window->frame_left + window->frame_right,
			       new_request.height + window->frame_top + window->frame_bottom);
	  bdk_window_resize (widget->window,
			     new_request.width, new_request.height);
	}
      
      if (window->type == BTK_WINDOW_POPUP)
        {
	  BtkAllocation allocation;

	  /* Directly size allocate for override redirect (popup) windows. */
          allocation.x = 0;
	  allocation.y = 0;
	  allocation.width = new_request.width;
	  allocation.height = new_request.height;

	  btk_widget_size_allocate (widget, &allocation);

	  bdk_window_process_updates (widget->window, TRUE);

	  if (container->resize_mode == BTK_RESIZE_QUEUE)
	    btk_widget_queue_draw (widget);
	}
      else
        {
	  /* Increment the number of have-not-yet-received-notify requests */
	  window->configure_request_count += 1;
	  bdk_window_freeze_toplevel_updates_libbtk_only (widget->window);

	  /* for BTK_RESIZE_QUEUE toplevels, we are now awaiting a new
	   * configure event in response to our resizing request.
	   * the configure event will cause a new resize with
	   * ->configure_notify_received=TRUE.
	   * until then, we want to
	   * - discard expose events
	   * - coalesce resizes for our children
	   * - defer any window resizes until the configure event arrived
	   * to achieve this, we queue a resize for the window, but remove its
	   * resizing handler, so resizing will not be handled from the next
	   * idle handler but when the configure event arrives.
	   *
	   * FIXME: we should also dequeue the pending redraws here, since
	   * we handle those ourselves upon ->configure_notify_received==TRUE.
	   */
	  if (container->resize_mode == BTK_RESIZE_QUEUE)
	    {
	      btk_widget_queue_resize_no_redraw (widget);
	      _btk_container_dequeue_resize_handler (container);
	    }
	}
    }
  else
    {
      /* Handle any position changes.
       */
      if (configure_request_pos_changed)
	{
	  if (window->frame)
	    {
	      bdk_window_move (window->frame,
			       new_request.x - window->frame_left,
			       new_request.y - window->frame_top);
	    }
	  else
	    bdk_window_move (widget->window,
			     new_request.x, new_request.y);
	}

      /* And run the resize queue.
       */
      btk_container_resize_children (container);
    }
  
  /* We have now processed a move/resize since the last position
   * constraint change, setting of the initial position, or resize.
   * (Not resetting these flags here can lead to infinite loops for
   * BTK_RESIZE_IMMEDIATE containers)
   */
  info->position_constraints_changed = FALSE;
  info->initial_pos_set = FALSE;
  info->resize_width = -1;
  info->resize_height = -1;
}

/* Compare two sets of Geometry hints for equality.
 */
static gboolean
btk_window_compare_hints (BdkGeometry *geometry_a,
			  guint        flags_a,
			  BdkGeometry *geometry_b,
			  guint        flags_b)
{
  if (flags_a != flags_b)
    return FALSE;
  
  if ((flags_a & BDK_HINT_MIN_SIZE) &&
      (geometry_a->min_width != geometry_b->min_width ||
       geometry_a->min_height != geometry_b->min_height))
    return FALSE;

  if ((flags_a & BDK_HINT_MAX_SIZE) &&
      (geometry_a->max_width != geometry_b->max_width ||
       geometry_a->max_height != geometry_b->max_height))
    return FALSE;

  if ((flags_a & BDK_HINT_BASE_SIZE) &&
      (geometry_a->base_width != geometry_b->base_width ||
       geometry_a->base_height != geometry_b->base_height))
    return FALSE;

  if ((flags_a & BDK_HINT_ASPECT) &&
      (geometry_a->min_aspect != geometry_b->min_aspect ||
       geometry_a->max_aspect != geometry_b->max_aspect))
    return FALSE;

  if ((flags_a & BDK_HINT_RESIZE_INC) &&
      (geometry_a->width_inc != geometry_b->width_inc ||
       geometry_a->height_inc != geometry_b->height_inc))
    return FALSE;

  if ((flags_a & BDK_HINT_WIN_GRAVITY) &&
      geometry_a->win_gravity != geometry_b->win_gravity)
    return FALSE;

  return TRUE;
}

void
_btk_window_constrain_size (BtkWindow   *window,
			    gint         width,
			    gint         height,
			    gint        *new_width,
			    gint        *new_height)
{
  BtkWindowGeometryInfo *info;

  g_return_if_fail (BTK_IS_WINDOW (window));

  info = window->geometry_info;
  if (info)
    {
      BdkWindowHints flags = info->last.flags;
      BdkGeometry *geometry = &info->last.geometry;
      
      btk_window_constrain_size (window,
				 geometry,
				 flags,
				 width,
				 height,
				 new_width,
				 new_height);
    }
}

static void 
btk_window_constrain_size (BtkWindow   *window,
			   BdkGeometry *geometry,
			   guint        flags,
			   gint         width,
			   gint         height,
			   gint        *new_width,
			   gint        *new_height)
{
  bdk_window_constrain_size (geometry, flags, width, height,
                             new_width, new_height);
}

/* Compute the set of geometry hints and flags for a window
 * based on the application set geometry, and requisiition
 * of the window. btk_widget_size_request() must have been
 * called first.
 */
static void
btk_window_compute_hints (BtkWindow   *window,
			  BdkGeometry *new_geometry,
			  guint       *new_flags)
{
  BtkWidget *widget;
  gint extra_width = 0;
  gint extra_height = 0;
  BtkWindowGeometryInfo *geometry_info;
  BtkRequisition requisition;

  widget = BTK_WIDGET (window);
  
  btk_widget_get_child_requisition (widget, &requisition);
  geometry_info = btk_window_get_geometry_info (BTK_WINDOW (widget), FALSE);

  if (geometry_info)
    {
      *new_flags = geometry_info->mask;
      *new_geometry = geometry_info->geometry;
    }
  else
    {
      *new_flags = 0;
    }
  
  if (geometry_info && geometry_info->widget)
    {
      BtkRequisition child_requisition;

      /* FIXME: This really isn't right. It gets the min size wrong and forces
       * callers to do horrible hacks like set a huge usize on the child requisition
       * to get the base size right. We really want to find the answers to:
       *
       *  - If the geometry widget was infinitely big, how much extra space
       *    would be needed for the stuff around it.
       *
       *  - If the geometry widget was infinitely small, how big would the
       *    window still have to be.
       *
       * Finding these answers would be a bit of a mess here. (Bug #68668)
       */
      btk_widget_get_child_requisition (geometry_info->widget, &child_requisition);
      
      extra_width = widget->requisition.width - child_requisition.width;
      extra_height = widget->requisition.height - child_requisition.height;
    }

  /* We don't want to set BDK_HINT_POS in here, we just set it
   * in btk_window_move_resize() when we want the position
   * honored.
   */
  
  if (*new_flags & BDK_HINT_BASE_SIZE)
    {
      new_geometry->base_width += extra_width;
      new_geometry->base_height += extra_height;
    }
  else if (!(*new_flags & BDK_HINT_MIN_SIZE) &&
	   (*new_flags & BDK_HINT_RESIZE_INC) &&
	   ((extra_width != 0) || (extra_height != 0)))
    {
      *new_flags |= BDK_HINT_BASE_SIZE;
      
      new_geometry->base_width = extra_width;
      new_geometry->base_height = extra_height;
    }
  
  if (*new_flags & BDK_HINT_MIN_SIZE)
    {
      if (new_geometry->min_width < 0)
	new_geometry->min_width = requisition.width;
      else
        new_geometry->min_width += extra_width;

      if (new_geometry->min_height < 0)
	new_geometry->min_height = requisition.height;
      else
	new_geometry->min_height += extra_height;
    }
  else if (!window->allow_shrink)
    {
      *new_flags |= BDK_HINT_MIN_SIZE;
      
      new_geometry->min_width = requisition.width;
      new_geometry->min_height = requisition.height;
    }
  
  if (*new_flags & BDK_HINT_MAX_SIZE)
    {
      if (new_geometry->max_width < 0)
	new_geometry->max_width = requisition.width;
      else
	new_geometry->max_width += extra_width;

      if (new_geometry->max_height < 0)
	new_geometry->max_height = requisition.height;
      else
	new_geometry->max_height += extra_height;
    }
  else if (!window->allow_grow)
    {
      *new_flags |= BDK_HINT_MAX_SIZE;
      
      new_geometry->max_width = requisition.width;
      new_geometry->max_height = requisition.height;
    }

  *new_flags |= BDK_HINT_WIN_GRAVITY;
  new_geometry->win_gravity = window->gravity;
}

/***********************
 * Redrawing functions *
 ***********************/

static void
btk_window_paint (BtkWidget     *widget,
		  BdkRectangle *area)
{
  btk_paint_flat_box (widget->style, widget->window, BTK_STATE_NORMAL, 
		      BTK_SHADOW_NONE, area, widget, "base", 0, 0, -1, -1);
}

static gint
btk_window_expose (BtkWidget      *widget,
		   BdkEventExpose *event)
{
  if (!btk_widget_get_app_paintable (widget))
    btk_window_paint (widget, &event->area);
  
  if (BTK_WIDGET_CLASS (btk_window_parent_class)->expose_event)
    return BTK_WIDGET_CLASS (btk_window_parent_class)->expose_event (widget, event);

  return FALSE;
}

/**
 * btk_window_set_has_frame:
 * @window: a #BtkWindow
 * @setting: a boolean
 *
 * (Note: this is a special-purpose function for the framebuffer port,
 *  that causes BTK+ to draw its own window border. For most applications,
 *  you want btk_window_set_decorated() instead, which tells the window
 *  manager whether to draw the window border.)
 * 
 * If this function is called on a window with setting of %TRUE, before
 * it is realized or showed, it will have a "frame" window around
 * @window->window, accessible in @window->frame. Using the signal 
 * frame_event you can receive all events targeted at the frame.
 * 
 * This function is used by the linux-fb port to implement managed
 * windows, but it could conceivably be used by X-programs that
 * want to do their own window decorations.
 *
 * Deprecated: 2.24: This function will be removed in BTK+ 3
 **/
void
btk_window_set_has_frame (BtkWindow *window, 
			  gboolean   setting)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (!btk_widget_get_realized (BTK_WIDGET (window)));

  window->has_frame = setting != FALSE;
}

/**
 * btk_window_get_has_frame:
 * @window: a #BtkWindow
 * 
 * Accessor for whether the window has a frame window exterior to
 * @window->window. Gets the value set by btk_window_set_has_frame ().
 *
 * Return value: %TRUE if a frame has been added to the window
 *   via btk_window_set_has_frame().
 *
 * Deprecated: 2.24: This function will be removed in BTK+ 3
 **/
gboolean
btk_window_get_has_frame (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  return window->has_frame;
}

/**
 * btk_window_set_frame_dimensions:
 * @window: a #BtkWindow that has a frame
 * @left: The width of the left border
 * @top: The height of the top border
 * @right: The width of the right border
 * @bottom: The height of the bottom border
 *
 * (Note: this is a special-purpose function intended for the framebuffer
 *  port; see btk_window_set_has_frame(). It will have no effect on the
 *  window border drawn by the window manager, which is the normal
 *  case when using the X Window system.)
 *
 * For windows with frames (see btk_window_set_has_frame()) this function
 * can be used to change the size of the frame border.
 *
 * Deprecated: 2.24: This function will be removed in BTK+ 3
 **/
void
btk_window_set_frame_dimensions (BtkWindow *window, 
				 gint       left,
				 gint       top,
				 gint       right,
				 gint       bottom)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  if (window->frame_left == left &&
      window->frame_top == top &&
      window->frame_right == right && 
      window->frame_bottom == bottom)
    return;

  window->frame_left = left;
  window->frame_top = top;
  window->frame_right = right;
  window->frame_bottom = bottom;

  if (btk_widget_get_realized (widget) && window->frame)
    {
      gint width = widget->allocation.width + left + right;
      gint height = widget->allocation.height + top + bottom;
      bdk_window_resize (window->frame, width, height);
      btk_decorated_window_move_resize_window (window,
					       left, top,
					       widget->allocation.width,
					       widget->allocation.height);
    }
}

/**
 * btk_window_present:
 * @window: a #BtkWindow
 *
 * Presents a window to the user. This may mean raising the window
 * in the stacking order, deiconifying it, moving it to the current
 * desktop, and/or giving it the keyboard focus, possibly dependent
 * on the user's platform, window manager, and preferences.
 *
 * If @window is hidden, this function calls btk_widget_show()
 * as well.
 * 
 * This function should be used when the user tries to open a window
 * that's already open. Say for example the preferences dialog is
 * currently open, and the user chooses Preferences from the menu
 * a second time; use btk_window_present() to move the already-open dialog
 * where the user can see it.
 *
 * If you are calling this function in response to a user interaction,
 * it is preferable to use btk_window_present_with_time().
 * 
 **/
void
btk_window_present (BtkWindow *window)
{
  btk_window_present_with_time (window, BDK_CURRENT_TIME);
}

/**
 * btk_window_present_with_time:
 * @window: a #BtkWindow
 * @timestamp: the timestamp of the user interaction (typically a 
 *   button or key press event) which triggered this call
 *
 * Presents a window to the user in response to a user interaction.
 * If you need to present a window without a timestamp, use 
 * btk_window_present(). See btk_window_present() for details. 
 * 
 * Since: 2.8
 **/
void
btk_window_present_with_time (BtkWindow *window,
			      guint32    timestamp)
{
  BtkWidget *widget;

  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  if (btk_widget_get_visible (widget))
    {
      g_assert (widget->window != NULL);
      
      bdk_window_show (widget->window);

      /* Translate a timestamp of BDK_CURRENT_TIME appropriately */
      if (timestamp == BDK_CURRENT_TIME)
        {
#ifdef BDK_WINDOWING_X11
          BdkDisplay *display;

          display = btk_widget_get_display (BTK_WIDGET (window));
          timestamp = bdk_x11_display_get_user_time (display);
#else
          timestamp = btk_get_current_event_time ();
#endif
        }

      bdk_window_focus (widget->window, timestamp);
    }
  else
    {
      btk_widget_show (widget);
    }
}

/**
 * btk_window_iconify:
 * @window: a #BtkWindow
 *
 * Asks to iconify (i.e. minimize) the specified @window. Note that
 * you shouldn't assume the window is definitely iconified afterward,
 * because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could deiconify it
 * again, or there may not be a window manager in which case
 * iconification isn't possible, etc. But normally the window will end
 * up iconified. Just don't write code that crashes if not.
 *
 * It's permitted to call this function before showing a window,
 * in which case the window will be iconified before it ever appears
 * onscreen.
 *
 * You can track iconification via the "window-state-event" signal
 * on #BtkWidget.
 * 
 **/
void
btk_window_iconify (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  window->iconify_initially = TRUE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_iconify (toplevel);
}

/**
 * btk_window_deiconify:
 * @window: a #BtkWindow
 *
 * Asks to deiconify (i.e. unminimize) the specified @window. Note
 * that you shouldn't assume the window is definitely deiconified
 * afterward, because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could iconify it
 * again before your code which assumes deiconification gets to run.
 *
 * You can track iconification via the "window-state-event" signal
 * on #BtkWidget.
 **/
void
btk_window_deiconify (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  window->iconify_initially = FALSE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_deiconify (toplevel);
}

/**
 * btk_window_stick:
 * @window: a #BtkWindow
 *
 * Asks to stick @window, which means that it will appear on all user
 * desktops. Note that you shouldn't assume the window is definitely
 * stuck afterward, because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could unstick it
 * again, and some window managers do not support sticking
 * windows. But normally the window will end up stuck. Just don't
 * write code that crashes if not.
 *
 * It's permitted to call this function before showing a window.
 *
 * You can track stickiness via the "window-state-event" signal
 * on #BtkWidget.
 * 
 **/
void
btk_window_stick (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  window->stick_initially = TRUE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_stick (toplevel);
}

/**
 * btk_window_unstick:
 * @window: a #BtkWindow
 *
 * Asks to unstick @window, which means that it will appear on only
 * one of the user's desktops. Note that you shouldn't assume the
 * window is definitely unstuck afterward, because other entities
 * (e.g. the user or <link linkend="btk-X11-arch">window
 * manager</link>) could stick it again. But normally the window will
 * end up stuck. Just don't write code that crashes if not.
 *
 * You can track stickiness via the "window-state-event" signal
 * on #BtkWidget.
 * 
 **/
void
btk_window_unstick (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  window->stick_initially = FALSE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_unstick (toplevel);
}

/**
 * btk_window_maximize:
 * @window: a #BtkWindow
 *
 * Asks to maximize @window, so that it becomes full-screen. Note that
 * you shouldn't assume the window is definitely maximized afterward,
 * because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could unmaximize it
 * again, and not all window managers support maximization. But
 * normally the window will end up maximized. Just don't write code
 * that crashes if not.
 *
 * It's permitted to call this function before showing a window,
 * in which case the window will be maximized when it appears onscreen
 * initially.
 *
 * You can track maximization via the "window-state-event" signal
 * on #BtkWidget.
 * 
 **/
void
btk_window_maximize (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  window->maximize_initially = TRUE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_maximize (toplevel);
}

/**
 * btk_window_unmaximize:
 * @window: a #BtkWindow
 *
 * Asks to unmaximize @window. Note that you shouldn't assume the
 * window is definitely unmaximized afterward, because other entities
 * (e.g. the user or <link linkend="btk-X11-arch">window
 * manager</link>) could maximize it again, and not all window
 * managers honor requests to unmaximize. But normally the window will
 * end up unmaximized. Just don't write code that crashes if not.
 *
 * You can track maximization via the "window-state-event" signal
 * on #BtkWidget.
 * 
 **/
void
btk_window_unmaximize (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);

  window->maximize_initially = FALSE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_unmaximize (toplevel);
}

/**
 * btk_window_fullscreen:
 * @window: a #BtkWindow
 *
 * Asks to place @window in the fullscreen state. Note that you
 * shouldn't assume the window is definitely full screen afterward,
 * because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could unfullscreen it
 * again, and not all window managers honor requests to fullscreen
 * windows. But normally the window will end up fullscreen. Just
 * don't write code that crashes if not.
 *
 * You can track the fullscreen state via the "window-state-event" signal
 * on #BtkWidget.
 * 
 * Since: 2.2
 **/
void
btk_window_fullscreen (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  BtkWindowPrivate *priv;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);
  priv = BTK_WINDOW_GET_PRIVATE (window);
  
  priv->fullscreen_initially = TRUE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_fullscreen (toplevel);
}

/**
 * btk_window_unfullscreen:
 * @window: a #BtkWindow
 *
 * Asks to toggle off the fullscreen state for @window. Note that you
 * shouldn't assume the window is definitely not full screen
 * afterward, because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could fullscreen it
 * again, and not all window managers honor requests to unfullscreen
 * windows. But normally the window will end up restored to its normal
 * state. Just don't write code that crashes if not.
 *
 * You can track the fullscreen state via the "window-state-event" signal
 * on #BtkWidget.
 * 
 * Since: 2.2
 **/
void
btk_window_unfullscreen (BtkWindow *window)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  BtkWindowPrivate *priv;
  
  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);
  priv = BTK_WINDOW_GET_PRIVATE (window);
  
  priv->fullscreen_initially = FALSE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    bdk_window_unfullscreen (toplevel);
}

/**
 * btk_window_set_keep_above:
 * @window: a #BtkWindow
 * @setting: whether to keep @window above other windows
 *
 * Asks to keep @window above, so that it stays on top. Note that
 * you shouldn't assume the window is definitely above afterward,
 * because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could not keep it above,
 * and not all window managers support keeping windows above. But
 * normally the window will end kept above. Just don't write code
 * that crashes if not.
 *
 * It's permitted to call this function before showing a window,
 * in which case the window will be kept above when it appears onscreen
 * initially.
 *
 * You can track the above state via the "window-state-event" signal
 * on #BtkWidget.
 *
 * Note that, according to the <ulink 
 * url="http://www.freedesktop.org/Standards/wm-spec">Extended Window 
 * Manager Hints</ulink> specification, the above state is mainly meant 
 * for user preferences and should not be used by applications e.g. for 
 * drawing attention to their dialogs.
 *
 * Since: 2.4
 **/
void
btk_window_set_keep_above (BtkWindow *window,
			   gboolean   setting)
{
  BtkWidget *widget;
  BtkWindowPrivate *priv;
  BdkWindow *toplevel;

  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);
  priv = BTK_WINDOW_GET_PRIVATE (window);

  priv->above_initially = setting != FALSE;
  if (setting)
    priv->below_initially = FALSE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;

  if (toplevel != NULL)
    bdk_window_set_keep_above (toplevel, setting);
}

/**
 * btk_window_set_keep_below:
 * @window: a #BtkWindow
 * @setting: whether to keep @window below other windows
 *
 * Asks to keep @window below, so that it stays in bottom. Note that
 * you shouldn't assume the window is definitely below afterward,
 * because other entities (e.g. the user or <link
 * linkend="btk-X11-arch">window manager</link>) could not keep it below,
 * and not all window managers support putting windows below. But
 * normally the window will be kept below. Just don't write code
 * that crashes if not.
 *
 * It's permitted to call this function before showing a window,
 * in which case the window will be kept below when it appears onscreen
 * initially.
 *
 * You can track the below state via the "window-state-event" signal
 * on #BtkWidget.
 *
 * Note that, according to the <ulink 
 * url="http://www.freedesktop.org/Standards/wm-spec">Extended Window 
 * Manager Hints</ulink> specification, the above state is mainly meant 
 * for user preferences and should not be used by applications e.g. for 
 * drawing attention to their dialogs.
 *
 * Since: 2.4
 **/
void
btk_window_set_keep_below (BtkWindow *window,
			   gboolean   setting)
{
  BtkWidget *widget;
  BtkWindowPrivate *priv;
  BdkWindow *toplevel;

  g_return_if_fail (BTK_IS_WINDOW (window));

  widget = BTK_WIDGET (window);
  priv = BTK_WINDOW_GET_PRIVATE (window);

  priv->below_initially = setting != FALSE;
  if (setting)
    priv->above_initially = FALSE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;

  if (toplevel != NULL)
    bdk_window_set_keep_below (toplevel, setting);
}

/**
 * btk_window_set_resizable:
 * @window: a #BtkWindow
 * @resizable: %TRUE if the user can resize this window
 *
 * Sets whether the user can resize a window. Windows are user resizable
 * by default.
 **/
void
btk_window_set_resizable (BtkWindow *window,
                          gboolean   resizable)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  btk_window_set_policy_internal (window, FALSE, resizable, FALSE);
}

/**
 * btk_window_get_resizable:
 * @window: a #BtkWindow
 *
 * Gets the value set by btk_window_set_resizable().
 *
 * Return value: %TRUE if the user can resize the window
 **/
gboolean
btk_window_get_resizable (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  /* allow_grow is most likely to indicate the semantic concept we
   * mean by "resizable" (and will be a reliable indicator if
   * set_policy() hasn't been called)
   */
  return window->allow_grow;
}

/**
 * btk_window_set_gravity:
 * @window: a #BtkWindow
 * @gravity: window gravity
 *
 * Window gravity defines the meaning of coordinates passed to
 * btk_window_move(). See btk_window_move() and #BdkGravity for
 * more details.
 *
 * The default window gravity is #BDK_GRAVITY_NORTH_WEST which will
 * typically "do what you mean."
 *
 **/
void
btk_window_set_gravity (BtkWindow *window,
			BdkGravity gravity)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (gravity != window->gravity)
    {
      window->gravity = gravity;

      /* btk_window_move_resize() will adapt gravity
       */
      btk_widget_queue_resize_no_redraw (BTK_WIDGET (window));

      g_object_notify (G_OBJECT (window), "gravity");
    }
}

/**
 * btk_window_get_gravity:
 * @window: a #BtkWindow
 *
 * Gets the value set by btk_window_set_gravity().
 *
 * Return value: (transfer none): window gravity
 **/
BdkGravity
btk_window_get_gravity (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), 0);

  return window->gravity;
}

/**
 * btk_window_begin_resize_drag:
 * @window: a #BtkWindow
 * @button: mouse button that initiated the drag
 * @edge: position of the resize control
 * @root_x: X position where the user clicked to initiate the drag, in root window coordinates
 * @root_y: Y position where the user clicked to initiate the drag
 * @timestamp: timestamp from the click event that initiated the drag
 *
 * Starts resizing a window. This function is used if an application
 * has window resizing controls. When BDK can support it, the resize
 * will be done using the standard mechanism for the <link
 * linkend="btk-X11-arch">window manager</link> or windowing
 * system. Otherwise, BDK will try to emulate window resizing,
 * potentially not all that well, depending on the windowing system.
 * 
 **/
void
btk_window_begin_resize_drag  (BtkWindow    *window,
                               BdkWindowEdge edge,
                               gint          button,
                               gint          root_x,
                               gint          root_y,
                               guint32       timestamp)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));
  widget = BTK_WIDGET (window);
  g_return_if_fail (btk_widget_get_visible (widget));
  
  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  bdk_window_begin_resize_drag (toplevel,
                                edge, button,
                                root_x, root_y,
                                timestamp);
}

/**
 * btk_window_get_frame_dimensions:
 * @window: a #BtkWindow
 * @left: (out) (allow-none): location to store the width of the frame at the left, or %NULL
 * @top: (out) (allow-none): location to store the height of the frame at the top, or %NULL
 * @right: (out) (allow-none): location to store the width of the frame at the returns, or %NULL
 * @bottom: (out) (allow-none): location to store the height of the frame at the bottom, or %NULL
 *
 * (Note: this is a special-purpose function intended for the
 *  framebuffer port; see btk_window_set_has_frame(). It will not
 *  return the size of the window border drawn by the <link
 *  linkend="btk-X11-arch">window manager</link>, which is the normal
 *  case when using a windowing system.  See
 *  bdk_window_get_frame_extents() to get the standard window border
 *  extents.)
 * 
 * Retrieves the dimensions of the frame window for this toplevel.
 * See btk_window_set_has_frame(), btk_window_set_frame_dimensions().
 *
 * Deprecated: 2.24: This function will be removed in BTK+ 3
 **/
void
btk_window_get_frame_dimensions (BtkWindow *window,
				 gint      *left,
				 gint      *top,
				 gint      *right,
				 gint      *bottom)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (left)
    *left = window->frame_left;
  if (top)
    *top = window->frame_top;
  if (right)
    *right = window->frame_right;
  if (bottom)
    *bottom = window->frame_bottom;
}

/**
 * btk_window_begin_move_drag:
 * @window: a #BtkWindow
 * @button: mouse button that initiated the drag
 * @root_x: X position where the user clicked to initiate the drag, in root window coordinates
 * @root_y: Y position where the user clicked to initiate the drag
 * @timestamp: timestamp from the click event that initiated the drag
 *
 * Starts moving a window. This function is used if an application has
 * window movement grips. When BDK can support it, the window movement
 * will be done using the standard mechanism for the <link
 * linkend="btk-X11-arch">window manager</link> or windowing
 * system. Otherwise, BDK will try to emulate window movement,
 * potentially not all that well, depending on the windowing system.
 * 
 **/
void
btk_window_begin_move_drag  (BtkWindow *window,
                             gint       button,
                             gint       root_x,
                             gint       root_y,
                             guint32    timestamp)
{
  BtkWidget *widget;
  BdkWindow *toplevel;
  
  g_return_if_fail (BTK_IS_WINDOW (window));
  widget = BTK_WIDGET (window);
  g_return_if_fail (btk_widget_get_visible (widget));
  
  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  bdk_window_begin_move_drag (toplevel,
                              button,
                              root_x, root_y,
                              timestamp);
}

/** 
 * btk_window_set_screen:
 * @window: a #BtkWindow.
 * @screen: a #BdkScreen.
 *
 * Sets the #BdkScreen where the @window is displayed; if
 * the window is already mapped, it will be unmapped, and
 * then remapped on the new screen.
 *
 * Since: 2.2
 */
void
btk_window_set_screen (BtkWindow *window,
		       BdkScreen *screen)
{
  BtkWidget *widget;
  BdkScreen *previous_screen;
  gboolean was_mapped;
  
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (BDK_IS_SCREEN (screen));

  if (screen == window->screen)
    return;

  widget = BTK_WIDGET (window);

  previous_screen = window->screen;
  was_mapped = btk_widget_get_mapped (widget);

  if (was_mapped)
    btk_widget_unmap (widget);
  if (btk_widget_get_realized (widget))
    btk_widget_unrealize (widget);
      
  btk_window_free_key_hash (window);
  window->screen = screen;
  btk_widget_reset_rc_styles (widget);
  if (screen != previous_screen)
    {
      g_signal_handlers_disconnect_by_func (previous_screen,
					    btk_window_on_composited_changed, window);
      g_signal_connect (screen, "composited-changed", 
			G_CALLBACK (btk_window_on_composited_changed), window);
      
      _btk_widget_propagate_screen_changed (widget, previous_screen);
      _btk_widget_propagate_composited_changed (widget);
    }
  g_object_notify (G_OBJECT (window), "screen");

  if (was_mapped)
    btk_widget_map (widget);
}

static void
btk_window_on_composited_changed (BdkScreen *screen,
				  BtkWindow *window)
{
  btk_widget_queue_draw (BTK_WIDGET (window));
  
  _btk_widget_propagate_composited_changed (BTK_WIDGET (window));
}

static BdkScreen *
btk_window_check_screen (BtkWindow *window)
{
  if (window->screen)
    return window->screen;
  else
    {
      g_warning ("Screen for BtkWindow not set; you must always set\n"
		 "a screen for a BtkWindow before using the window");
      return NULL;
    }
}

/**
 * btk_window_get_screen:
 * @window: a #BtkWindow.
 *
 * Returns the #BdkScreen associated with @window.
 *
 * Return value: (transfer none): a #BdkScreen.
 *
 * Since: 2.2
 */
BdkScreen*
btk_window_get_screen (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), NULL);
   
  return window->screen;
}

/**
 * btk_window_is_active:
 * @window: a #BtkWindow
 * 
 * Returns whether the window is part of the current active toplevel.
 * (That is, the toplevel window receiving keystrokes.)
 * The return value is %TRUE if the window is active toplevel
 * itself, but also if it is, say, a #BtkPlug embedded in the active toplevel.
 * You might use this function if you wanted to draw a widget
 * differently in an active window from a widget in an inactive window.
 * See btk_window_has_toplevel_focus()
 * 
 * Return value: %TRUE if the window part of the current active window.
 *
 * Since: 2.4
 **/
gboolean
btk_window_is_active (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  return window->is_active;
}

/**
 * btk_window_has_toplevel_focus:
 * @window: a #BtkWindow
 * 
 * Returns whether the input focus is within this BtkWindow.
 * For real toplevel windows, this is identical to btk_window_is_active(),
 * but for embedded windows, like #BtkPlug, the results will differ.
 * 
 * Return value: %TRUE if the input focus is within this BtkWindow
 *
 * Since: 2.4
 **/
gboolean
btk_window_has_toplevel_focus (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  return window->has_toplevel_focus;
}

static void
btk_window_group_class_init (BtkWindowGroupClass *klass)
{
}

GType
btk_window_group_get_type (void)
{
  static GType window_group_type = 0;

  if (!window_group_type)
    {
      const GTypeInfo window_group_info =
      {
	sizeof (BtkWindowGroupClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) btk_window_group_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (BtkWindowGroup),
	0,		/* n_preallocs */
	(GInstanceInitFunc) NULL,
      };

      window_group_type = g_type_register_static (G_TYPE_OBJECT, I_("BtkWindowGroup"), 
						  &window_group_info, 0);
    }

  return window_group_type;
}

/**
 * btk_window_group_new:
 * 
 * Creates a new #BtkWindowGroup object. Grabs added with
 * btk_grab_add() only affect windows within the same #BtkWindowGroup.
 * 
 * Return value: a new #BtkWindowGroup. 
 **/
BtkWindowGroup *
btk_window_group_new (void)
{
  return g_object_new (BTK_TYPE_WINDOW_GROUP, NULL);
}

static void
window_group_cleanup_grabs (BtkWindowGroup *group,
			    BtkWindow      *window)
{
  GSList *tmp_list;
  GSList *to_remove = NULL;

  tmp_list = group->grabs;
  while (tmp_list)
    {
      if (btk_widget_get_toplevel (tmp_list->data) == (BtkWidget*) window)
	to_remove = g_slist_prepend (to_remove, g_object_ref (tmp_list->data));
      tmp_list = tmp_list->next;
    }

  while (to_remove)
    {
      btk_grab_remove (to_remove->data);
      g_object_unref (to_remove->data);
      to_remove = g_slist_delete_link (to_remove, to_remove);
    }
}

/**
 * btk_window_group_add_window:
 * @window_group: a #BtkWindowGroup
 * @window: the #BtkWindow to add
 * 
 * Adds a window to a #BtkWindowGroup. 
 **/
void
btk_window_group_add_window (BtkWindowGroup *window_group,
			     BtkWindow      *window)
{
  g_return_if_fail (BTK_IS_WINDOW_GROUP (window_group));
  g_return_if_fail (BTK_IS_WINDOW (window));

  if (window->group != window_group)
    {
      g_object_ref (window);
      g_object_ref (window_group);
      
      if (window->group)
	btk_window_group_remove_window (window->group, window);
      else
	window_group_cleanup_grabs (btk_window_get_group (NULL), window);

      window->group = window_group;

      g_object_unref (window);
    }
}

/**
 * btk_window_group_remove_window:
 * @window_group: a #BtkWindowGroup
 * @window: the #BtkWindow to remove
 * 
 * Removes a window from a #BtkWindowGroup.
 **/
void
btk_window_group_remove_window (BtkWindowGroup *window_group,
				BtkWindow      *window)
{
  g_return_if_fail (BTK_IS_WINDOW_GROUP (window_group));
  g_return_if_fail (BTK_IS_WINDOW (window));
  g_return_if_fail (window->group == window_group);

  g_object_ref (window);

  window_group_cleanup_grabs (window_group, window);
  window->group = NULL;
  
  g_object_unref (window_group);
  g_object_unref (window);
}

/**
 * btk_window_group_list_windows:
 * @window_group: a #BtkWindowGroup
 *
 * Returns a list of the #BtkWindows that belong to @window_group.
 *
 * Returns: (element-type BtkWidget) (transfer container): A newly-allocated list of
 *   windows inside the group.
 *
 * Since: 2.14
 **/
GList *
btk_window_group_list_windows (BtkWindowGroup *window_group)
{
  GList *toplevels, *toplevel, *group_windows;

  g_return_val_if_fail (BTK_IS_WINDOW_GROUP (window_group), NULL);

  group_windows = NULL;
  toplevels = btk_window_list_toplevels ();

  for (toplevel = toplevels; toplevel; toplevel = toplevel->next)
    {
      BtkWindow *window = toplevel->data;

      if (window_group == window->group)
	group_windows = g_list_prepend (group_windows, window);
    }

  return g_list_reverse (group_windows);
}

/**
 * btk_window_get_group:
 * @window: (allow-none): a #BtkWindow, or %NULL
 *
 * Returns the group for @window or the default group, if
 * @window is %NULL or if @window does not have an explicit
 * window group.
 *
 * Returns: (transfer none): the #BtkWindowGroup for a window or the default group
 *
 * Since: 2.10
 */
BtkWindowGroup *
btk_window_get_group (BtkWindow *window)
{
  if (window && window->group)
    return window->group;
  else
    {
      static BtkWindowGroup *default_group = NULL;

      if (!default_group)
	default_group = btk_window_group_new ();

      return default_group;
    }
}

/**
 * btk_window_has_group:
 * @window: a #BtkWindow
 *
 * Returns whether @window has an explicit window group.
 *
 * Return value: %TRUE if @window has an explicit window group.
 *
 * Since 2.22
 **/
gboolean
btk_window_has_group (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  return window->group != NULL;
}

/**
 * btk_window_group_get_current_current_grab:
 * @window_group: a #BtkWindowGroup
 *
 * Gets the current grab widget of the given group,
 * see btk_grab_add().
 *
 * Returns: (transfer none): the current grab widget of the group
 *
 * Since: 2.22
 */
BtkWidget *
btk_window_group_get_current_grab (BtkWindowGroup *window_group)
{
  if (window_group->grabs)
    return BTK_WIDGET (window_group->grabs->data);
  return NULL;
}

/*
  Derived from XParseGeometry() in XFree86  

  Copyright 1985, 1986, 1987,1998  The Open Group

  All Rights Reserved.

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  Except as contained in this notice, the name of The Open Group shall
  not be used in advertising or otherwise to promote the sale, use or
  other dealings in this Software without prior written authorization
  from The Open Group.
*/


/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  "=80x24+300-49"
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged. 
 */

/* The following code is from Xlib, and is minimally modified, so we
 * can track any upstream changes if required.  Don't change this
 * code. Or if you do, put in a huge comment marking which thing
 * changed.
 */

static int
read_int (gchar   *string,
          gchar  **next)
{
  int result = 0;
  int sign = 1;
  
  if (*string == '+')
    string++;
  else if (*string == '-')
    {
      string++;
      sign = -1;
    }

  for (; (*string >= '0') && (*string <= '9'); string++)
    {
      result = (result * 10) + (*string - '0');
    }

  *next = string;

  if (sign >= 0)
    return (result);
  else
    return (-result);
}

/* 
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
#define NoValue         0x0000
#define XValue          0x0001
#define YValue          0x0002
#define WidthValue      0x0004
#define HeightValue     0x0008
#define AllValues       0x000F
#define XNegative       0x0010
#define YNegative       0x0020

/* Try not to reformat/modify, so we can compare/sync with X sources */
static int
btk_XParseGeometry (const char   *string,
                    int          *x,
                    int          *y,
                    unsigned int *width,   
                    unsigned int *height)  
{
  int mask = NoValue;
  char *strind;
  unsigned int tempWidth, tempHeight;
  int tempX, tempY;
  char *nextCharacter;

  /* These initializations are just to silence gcc */
  tempWidth = 0;
  tempHeight = 0;
  tempX = 0;
  tempY = 0;
  
  if ( (string == NULL) || (*string == '\0')) return(mask);
  if (*string == '=')
    string++;  /* ignore possible '=' at beg of geometry spec */

  strind = (char *)string;
  if (*strind != '+' && *strind != '-' && *strind != 'x') {
    tempWidth = read_int(strind, &nextCharacter);
    if (strind == nextCharacter) 
      return (0);
    strind = nextCharacter;
    mask |= WidthValue;
  }

  if (*strind == 'x' || *strind == 'X') {	
    strind++;
    tempHeight = read_int(strind, &nextCharacter);
    if (strind == nextCharacter)
      return (0);
    strind = nextCharacter;
    mask |= HeightValue;
  }

  if ((*strind == '+') || (*strind == '-')) {
    if (*strind == '-') {
      strind++;
      tempX = -read_int(strind, &nextCharacter);
      if (strind == nextCharacter)
        return (0);
      strind = nextCharacter;
      mask |= XNegative;

    }
    else
      {	strind++;
      tempX = read_int(strind, &nextCharacter);
      if (strind == nextCharacter)
        return(0);
      strind = nextCharacter;
      }
    mask |= XValue;
    if ((*strind == '+') || (*strind == '-')) {
      if (*strind == '-') {
        strind++;
        tempY = -read_int(strind, &nextCharacter);
        if (strind == nextCharacter)
          return(0);
        strind = nextCharacter;
        mask |= YNegative;

      }
      else
        {
          strind++;
          tempY = read_int(strind, &nextCharacter);
          if (strind == nextCharacter)
            return(0);
          strind = nextCharacter;
        }
      mask |= YValue;
    }
  }
	
  /* If strind isn't at the end of the string the it's an invalid
		geometry specification. */

  if (*strind != '\0') return (0);

  if (mask & XValue)
    *x = tempX;
  if (mask & YValue)
    *y = tempY;
  if (mask & WidthValue)
    *width = tempWidth;
  if (mask & HeightValue)
    *height = tempHeight;
  return (mask);
}

/**
 * btk_window_parse_geometry:
 * @window: a #BtkWindow
 * @geometry: geometry string
 * 
 * Parses a standard X Window System geometry string - see the
 * manual page for X (type 'man X') for details on this.
 * btk_window_parse_geometry() does work on all BTK+ ports
 * including Win32 but is primarily intended for an X environment.
 *
 * If either a size or a position can be extracted from the
 * geometry string, btk_window_parse_geometry() returns %TRUE
 * and calls btk_window_set_default_size() and/or btk_window_move()
 * to resize/move the window.
 *
 * If btk_window_parse_geometry() returns %TRUE, it will also
 * set the #BDK_HINT_USER_POS and/or #BDK_HINT_USER_SIZE hints
 * indicating to the window manager that the size/position of
 * the window was user-specified. This causes most window
 * managers to honor the geometry.
 *
 * Note that for btk_window_parse_geometry() to work as expected, it has
 * to be called when the window has its "final" size, i.e. after calling
 * btk_widget_show_all() on the contents and btk_window_set_geometry_hints()
 * on the window.
 * |[
 * #include <btk/btk.h>
 *    
 * static void
 * fill_with_content (BtkWidget *vbox)
 * {
 *   /&ast; fill with content... &ast;/
 * }
 *    
 * int
 * main (int argc, char *argv[])
 * {
 *   BtkWidget *window, *vbox;
 *   BdkGeometry size_hints = {
 *     100, 50, 0, 0, 100, 50, 10, 10, 0.0, 0.0, BDK_GRAVITY_NORTH_WEST  
 *   };
 *    
 *   btk_init (&argc, &argv);
 *   
 *   window = btk_window_new (BTK_WINDOW_TOPLEVEL);
 *   vbox = btk_vbox_new (FALSE, 0);
 *   
 *   btk_container_add (BTK_CONTAINER (window), vbox);
 *   fill_with_content (vbox);
 *   btk_widget_show_all (vbox);
 *   
 *   btk_window_set_geometry_hints (BTK_WINDOW (window),
 * 	  			    window,
 * 				    &size_hints,
 * 				    BDK_HINT_MIN_SIZE | 
 * 				    BDK_HINT_BASE_SIZE | 
 * 				    BDK_HINT_RESIZE_INC);
 *   
 *   if (argc &gt; 1)
 *     {
 *       if (!btk_window_parse_geometry (BTK_WINDOW (window), argv[1]))
 *         fprintf (stderr, "Failed to parse '%s'\n", argv[1]);
 *     }
 *    
 *   btk_widget_show_all (window);
 *   btk_main ();
 *    
 *   return 0;
 * }
 * ]|
 *
 * Return value: %TRUE if string was parsed successfully
 **/
gboolean
btk_window_parse_geometry (BtkWindow   *window,
                           const gchar *geometry)
{
  gint result, x = 0, y = 0;
  guint w, h;
  BdkGravity grav;
  gboolean size_set, pos_set;
  BdkScreen *screen;
  
  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (geometry != NULL, FALSE);

  screen = btk_window_check_screen (window);
  
  result = btk_XParseGeometry (geometry, &x, &y, &w, &h);

  size_set = FALSE;
  if ((result & WidthValue) || (result & HeightValue))
    {
      btk_window_set_default_size_internal (window, 
					    TRUE, result & WidthValue ? w : -1,
					    TRUE, result & HeightValue ? h : -1, 
					    TRUE);
      size_set = TRUE;
    }

  btk_window_get_size (window, (gint *)&w, (gint *)&h);
  
  grav = BDK_GRAVITY_NORTH_WEST;

  if ((result & XNegative) && (result & YNegative))
    grav = BDK_GRAVITY_SOUTH_EAST;
  else if (result & XNegative)
    grav = BDK_GRAVITY_NORTH_EAST;
  else if (result & YNegative)
    grav = BDK_GRAVITY_SOUTH_WEST;

  if ((result & XValue) == 0)
    x = 0;

  if ((result & YValue) == 0)
    y = 0;

  if (grav == BDK_GRAVITY_SOUTH_WEST ||
      grav == BDK_GRAVITY_SOUTH_EAST)
    y = bdk_screen_get_height (screen) - h + y;

  if (grav == BDK_GRAVITY_SOUTH_EAST ||
      grav == BDK_GRAVITY_NORTH_EAST)
    x = bdk_screen_get_width (screen) - w + x;

  /* we don't let you put a window offscreen; maybe some people would
   * prefer to be able to, but it's kind of a bogus thing to do.
   */
  if (y < 0)
    y = 0;

  if (x < 0)
    x = 0;

  pos_set = FALSE;
  if ((result & XValue) || (result & YValue))
    {
      btk_window_set_gravity (window, grav);
      btk_window_move (window, x, y);
      pos_set = TRUE;
    }

  if (size_set || pos_set)
    {
      /* Set USSize, USPosition hints */
      BtkWindowGeometryInfo *info;

      info = btk_window_get_geometry_info (window, TRUE);

      if (pos_set)
        info->mask |= BDK_HINT_USER_POS;
      if (size_set)
        info->mask |= BDK_HINT_USER_SIZE;
    }
  
  return result != 0;
}

static void
btk_window_mnemonic_hash_foreach (guint      keyval,
				  GSList    *targets,
				  gpointer   data)
{
  struct {
    BtkWindow *window;
    BtkWindowKeysForeachFunc func;
    gpointer func_data;
  } *info = data;

  (*info->func) (info->window, keyval, info->window->mnemonic_modifier, TRUE, info->func_data);
}

void
_btk_window_keys_foreach (BtkWindow                *window,
			  BtkWindowKeysForeachFunc func,
			  gpointer                 func_data)
{
  GSList *groups;
  BtkMnemonicHash *mnemonic_hash;

  struct {
    BtkWindow *window;
    BtkWindowKeysForeachFunc func;
    gpointer func_data;
  } info;

  info.window = window;
  info.func = func;
  info.func_data = func_data;

  mnemonic_hash = btk_window_get_mnemonic_hash (window, FALSE);
  if (mnemonic_hash)
    _btk_mnemonic_hash_foreach (mnemonic_hash,
				btk_window_mnemonic_hash_foreach, &info);

  groups = btk_accel_groups_from_object (G_OBJECT (window));
  while (groups)
    {
      BtkAccelGroup *group = groups->data;
      gint i;

      for (i = 0; i < group->n_accels; i++)
	{
	  BtkAccelKey *key = &group->priv_accels[i].key;
	  
	  if (key->accel_key)
	    (*func) (window, key->accel_key, key->accel_mods, FALSE, func_data);
	}
      
      groups = groups->next;
    }
}

static void
btk_window_keys_changed (BtkWindow *window)
{
  btk_window_free_key_hash (window);
  btk_window_get_key_hash (window);
}

typedef struct _BtkWindowKeyEntry BtkWindowKeyEntry;

struct _BtkWindowKeyEntry
{
  guint keyval;
  guint modifiers;
  guint is_mnemonic : 1;
};

static void 
window_key_entry_destroy (gpointer data)
{
  g_slice_free (BtkWindowKeyEntry, data);
}

static void
add_to_key_hash (BtkWindow      *window,
		 guint           keyval,
		 BdkModifierType modifiers,
		 gboolean        is_mnemonic,
		 gpointer        data)
{
  BtkKeyHash *key_hash = data;

  BtkWindowKeyEntry *entry = g_slice_new (BtkWindowKeyEntry);

  entry->keyval = keyval;
  entry->modifiers = modifiers;
  entry->is_mnemonic = is_mnemonic;

  /* BtkAccelGroup stores lowercased accelerators. To deal
   * with this, if <Shift> was specified, uppercase.
   */
  if (modifiers & BDK_SHIFT_MASK)
    {
      if (keyval == BDK_Tab)
	keyval = BDK_ISO_Left_Tab;
      else
	keyval = bdk_keyval_to_upper (keyval);
    }
  
  _btk_key_hash_add_entry (key_hash, keyval, entry->modifiers, entry);
}

static BtkKeyHash *
btk_window_get_key_hash (BtkWindow *window)
{
  BdkScreen *screen = btk_window_check_screen (window);
  BtkKeyHash *key_hash = g_object_get_qdata (G_OBJECT (window), quark_btk_window_key_hash);
  
  if (key_hash)
    return key_hash;
  
  key_hash = _btk_key_hash_new (bdk_keymap_get_for_display (bdk_screen_get_display (screen)),
				(GDestroyNotify)window_key_entry_destroy);
  _btk_window_keys_foreach (window, add_to_key_hash, key_hash);
  g_object_set_qdata (G_OBJECT (window), quark_btk_window_key_hash, key_hash);

  return key_hash;
}

static void
btk_window_free_key_hash (BtkWindow *window)
{
  BtkKeyHash *key_hash = g_object_get_qdata (G_OBJECT (window), quark_btk_window_key_hash);
  if (key_hash)
    {
      _btk_key_hash_free (key_hash);
      g_object_set_qdata (G_OBJECT (window), quark_btk_window_key_hash, NULL);
    }
}

/**
 * btk_window_activate_key:
 * @window:  a #BtkWindow
 * @event:   a #BdkEventKey
 *
 * Activates mnemonics and accelerators for this #BtkWindow. This is normally
 * called by the default ::key_press_event handler for toplevel windows,
 * however in some cases it may be useful to call this directly when
 * overriding the standard key handling for a toplevel window.
 *
 * Return value: %TRUE if a mnemonic or accelerator was found and activated.
 *
 * Since: 2.4
 */
gboolean
btk_window_activate_key (BtkWindow   *window,
			 BdkEventKey *event)
{
  BtkKeyHash *key_hash;
  BtkWindowKeyEntry *found_entry = NULL;
  gboolean enable_mnemonics;
  gboolean enable_accels;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  key_hash = btk_window_get_key_hash (window);

  if (key_hash)
    {
      GSList *tmp_list;
      GSList *entries = _btk_key_hash_lookup (key_hash,
					      event->hardware_keycode,
					      event->state,
					      btk_accelerator_get_default_mod_mask (),
					      event->group);

      g_object_get (btk_widget_get_settings (BTK_WIDGET (window)),
                    "btk-enable-mnemonics", &enable_mnemonics,
                    "btk-enable-accels", &enable_accels,
                    NULL);

      for (tmp_list = entries; tmp_list; tmp_list = tmp_list->next)
	{
	  BtkWindowKeyEntry *entry = tmp_list->data;
	  if (entry->is_mnemonic)
            {
              if (enable_mnemonics)
	        {
	          found_entry = entry;
	          break;
	        }
            }
          else 
            {
              if (enable_accels && !found_entry)
                {
	          found_entry = entry;
                }
            }
	}

      g_slist_free (entries);
    }

  if (found_entry)
    {
      if (found_entry->is_mnemonic)
        {
          if (enable_mnemonics)
            return btk_window_mnemonic_activate (window, found_entry->keyval,
                                                 found_entry->modifiers);
        }
      else
        {
          if (enable_accels)
            return btk_accel_groups_activate (G_OBJECT (window), found_entry->keyval,
                                              found_entry->modifiers);
        }
    }

  return FALSE;
}

static void
window_update_has_focus (BtkWindow *window)
{
  BtkWidget *widget = BTK_WIDGET (window);
  gboolean has_focus = window->has_toplevel_focus && window->is_active;
  
  if (has_focus != window->has_focus)
    {
      window->has_focus = has_focus;
      
      if (has_focus)
	{
	  if (window->focus_widget &&
	      window->focus_widget != widget &&
	      !btk_widget_has_focus (window->focus_widget))
	    do_focus_change (window->focus_widget, TRUE);	
	}
      else
	{
	  if (window->focus_widget &&
	      window->focus_widget != widget &&
	      btk_widget_has_focus (window->focus_widget))
	    do_focus_change (window->focus_widget, FALSE);
	}
    }
}

/**
 * _btk_window_set_is_active:
 * @window: a #BtkWindow
 * @is_active: %TRUE if the window is in the currently active toplevel
 * 
 * Internal function that sets whether the #BtkWindow is part
 * of the currently active toplevel window (taking into account inter-process
 * embedding.)
 **/
void
_btk_window_set_is_active (BtkWindow *window,
			   gboolean   is_active)
{
  g_return_if_fail (BTK_IS_WINDOW (window));

  is_active = is_active != FALSE;

  if (is_active != window->is_active)
    {
      window->is_active = is_active;
      window_update_has_focus (window);

      g_object_notify (G_OBJECT (window), "is-active");
    }
}

/**
 * _btk_window_set_is_toplevel:
 * @window: a #BtkWindow
 * @is_toplevel: %TRUE if the window is still a real toplevel (nominally a
 * parent of the root window); %FALSE if it is not (for example, for an
 * in-process, parented BtkPlug)
 *
 * Internal function used by #BtkPlug when it gets parented/unparented by a
 * #BtkSocket.  This keeps the @window's #BTK_TOPLEVEL flag in sync with the
 * global list of toplevel windows.
 */
void
_btk_window_set_is_toplevel (BtkWindow *window,
			     gboolean   is_toplevel)
{
  BtkWidget *widget;

  widget = BTK_WIDGET (window);

  if (btk_widget_is_toplevel (widget))
    g_assert (g_slist_find (toplevel_list, window) != NULL);
  else
    g_assert (g_slist_find (toplevel_list, window) == NULL);

  if (is_toplevel == btk_widget_is_toplevel (widget))
    return;

  if (is_toplevel)
    {
      _btk_widget_set_is_toplevel (widget, TRUE);
      toplevel_list = g_slist_prepend (toplevel_list, window);
    }
  else
    {
      _btk_widget_set_is_toplevel (widget, FALSE);
      toplevel_list = g_slist_remove (toplevel_list, window);
    }
}

/**
 * _btk_window_set_has_toplevel_focus:
 * @window: a #BtkWindow
 * @has_toplevel_focus: %TRUE if the in
 * 
 * Internal function that sets whether the keyboard focus for the
 * toplevel window (taking into account inter-process embedding.)
 **/
void
_btk_window_set_has_toplevel_focus (BtkWindow *window,
				   gboolean   has_toplevel_focus)
{
  g_return_if_fail (BTK_IS_WINDOW (window));
  
  has_toplevel_focus = has_toplevel_focus != FALSE;

  if (has_toplevel_focus != window->has_toplevel_focus)
    {
      window->has_toplevel_focus = has_toplevel_focus;
      window_update_has_focus (window);

      g_object_notify (G_OBJECT (window), "has-toplevel-focus");
    }
}

/**
 * btk_window_set_auto_startup_notification:
 * @setting: %TRUE to automatically do startup notification
 *
 * By default, after showing the first #BtkWindow, BTK+ calls 
 * bdk_notify_startup_complete().  Call this function to disable 
 * the automatic startup notification. You might do this if your 
 * first window is a splash screen, and you want to delay notification 
 * until after your real main window has been shown, for example.
 *
 * In that example, you would disable startup notification
 * temporarily, show your splash screen, then re-enable it so that
 * showing the main window would automatically result in notification.
 * 
 * Since: 2.2
 **/
void
btk_window_set_auto_startup_notification (gboolean setting)
{
  disable_startup_notification = !setting;
}

/**
 * btk_window_get_window_type:
 * @window: a #BtkWindow
 *
 * Gets the type of the window. See #BtkWindowType.
 *
 * Return value: the type of the window
 *
 * Since: 2.20
 **/
BtkWindowType
btk_window_get_window_type (BtkWindow *window)
{
  g_return_val_if_fail (BTK_IS_WINDOW (window), BTK_WINDOW_TOPLEVEL);

  return window->type;
}

/* btk_window_get_mnemonics_visible:
 * @window: a #BtkWindow
 *
 * Gets the value of the #BtkWindow:mnemonics-visible property.
 *
 * Returns: %TRUE if mnemonics are supposed to be visible
 * in this window.
 *
 * Since: 2.20
 */
gboolean
btk_window_get_mnemonics_visible (BtkWindow *window)
{
  BtkWindowPrivate *priv;

  g_return_val_if_fail (BTK_IS_WINDOW (window), FALSE);

  priv = BTK_WINDOW_GET_PRIVATE (window);

  return priv->mnemonics_visible;
}

/**
 * btk_window_set_mnemonics_visible:
 * @window: a #BtkWindow
 * @setting: the new value
 *
 * Sets the #BtkWindow:mnemonics-visible property.
 *
 * Since: 2.20
 */
void
btk_window_set_mnemonics_visible (BtkWindow *window,
                                  gboolean   setting)
{
  BtkWindowPrivate *priv;

  g_return_if_fail (BTK_IS_WINDOW (window));

  priv = BTK_WINDOW_GET_PRIVATE (window);

  setting = setting != FALSE;

  if (priv->mnemonics_visible != setting)
    {
      priv->mnemonics_visible = setting;
      g_object_notify (G_OBJECT (window), "mnemonics-visible");
    }

  priv->mnemonics_visible_set = TRUE;
}

#if defined (G_OS_WIN32) && !defined (_WIN64)

#undef btk_window_set_icon_from_file

gboolean
btk_window_set_icon_from_file (BtkWindow   *window,
			       const gchar *filename,
			       GError     **err)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, err);
  gboolean retval;

  if (utf8_filename == NULL)
    return FALSE;

  retval = btk_window_set_icon_from_file_utf8 (window, utf8_filename, err);

  g_free (utf8_filename);

  return retval;
}

#undef btk_window_set_default_icon_from_file

gboolean
btk_window_set_default_icon_from_file (const gchar *filename,
				       GError     **err)
{
  gchar *utf8_filename = g_locale_to_utf8 (filename, -1, NULL, NULL, err);
  gboolean retval;

  if (utf8_filename == NULL)
    return FALSE;

  retval = btk_window_set_default_icon_from_file_utf8 (utf8_filename, err);

  g_free (utf8_filename);

  return retval;
}

#endif

#define __BTK_WINDOW_C__
#include "btkaliasdef.c"
