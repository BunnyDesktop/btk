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
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <stdarg.h>
#include <string.h>
#include <locale.h>
#include "btkcontainer.h"
#include "btkaccelmap.h"
#include "btkclipboard.h"
#include "btkiconfactory.h"
#include "btkintl.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkrc.h"
#include "btkselection.h"
#include "btksettings.h"
#include "btksizegroup.h"
#include "btkwidget.h"
#include "btkwindow.h"
#include "btkbindings.h"
#include "btkprivate.h"
#include "bdk/bdk.h"
#include "bdk/bdkprivate.h" /* Used in btk_reset_shapes_recurse to avoid copy */
#include <bobject/gvaluecollector.h>
#include <bobject/bobjectnotifyqueue.c>
#include "bdk/bdkkeysyms.h"
#include "btkaccessible.h"
#include "btktooltip.h"
#include "btkinvisible.h"
#include "btkbuildable.h"
#include "btkbuilderprivate.h"
#include "btkalias.h"

/**
 * SECTION:btkwidget
 * @Short_description: Base class for all widgets
 * @Title: BtkWidget
 *
 * BtkWidget is the base class all widgets in BTK+ derive from. It manages the
 * widget lifecycle, states and style.
 *
 * <refsect2 id="style-properties">
 * <para>
 * <structname>BtkWidget</structname> introduces <firstterm>style
 * properties</firstterm> - these are basically object properties that are stored
 * not on the object, but in the style object associated to the widget. Style
 * properties are set in <link linkend="btk-Resource-Files">resource files</link>.
 * This mechanism is used for configuring such things as the location of the
 * scrollbar arrows through the theme, giving theme authors more control over the
 * look of applications without the need to write a theme engine in C.
 * </para>
 * <para>
 * Use btk_widget_class_install_style_property() to install style properties for
 * a widget class, btk_widget_class_find_style_property() or
 * btk_widget_class_list_style_properties() to get information about existing
 * style properties and btk_widget_style_get_property(), btk_widget_style_get() or
 * btk_widget_style_get_valist() to obtain the value of a style property.
 * </para>
 * </refsect2>
 * <refsect2 id="BtkWidget-BUILDER-UI">
 * <title>BtkWidget as BtkBuildable</title>
 * <para>
 * The BtkWidget implementation of the BtkBuildable interface supports a
 * custom &lt;accelerator&gt; element, which has attributes named key,
 * modifiers and signal and allows to specify accelerators.
 * </para>
 * <example>
 * <title>A UI definition fragment specifying an accelerator</title>
 * <programlisting><![CDATA[
 * <object class="BtkButton">
 *   <accelerator key="q" modifiers="BDK_CONTROL_MASK" signal="clicked"/>
 * </object>
 * ]]></programlisting>
 * </example>
 * <para>
 * In addition to accelerators, <structname>BtkWidget</structname> also support a
 * custom &lt;accessible&gt; element, which supports actions and relations.
 * Properties on the accessible implementation of an object can be set by accessing the
 * internal child "accessible" of a <structname>BtkWidget</structname>.
 * </para>
 * <example>
 * <title>A UI definition fragment specifying an accessible</title>
 * <programlisting><![CDATA[
 * <object class="BtkButton" id="label1"/>
 *   <property name="label">I am a Label for a Button</property>
 * </object>
 * <object class="BtkButton" id="button1">
 *   <accessibility>
 *     <action action_name="click" translatable="yes">Click the button.</action>
 *     <relation target="label1" type="labelled-by"/>
 *   </accessibility>
 *   <child internal-child="accessible">
 *     <object class="BatkObject" id="a11y-button1">
 *       <property name="BatkObject::name">Clickable Button</property>
 *     </object>
 *   </child>
 * </object>
 * ]]></programlisting>
 * </example>
 * </refsect2>
 */

#define WIDGET_CLASS(w)	 BTK_WIDGET_GET_CLASS (w)
#define	INIT_PATH_SIZE	(512)


enum {
  SHOW,
  HIDE,
  MAP,
  UNMAP,
  REALIZE,
  UNREALIZE,
  SIZE_REQUEST,
  SIZE_ALLOCATE,
  STATE_CHANGED,
  PARENT_SET,
  HIERARCHY_CHANGED,
  STYLE_SET,
  DIRECTION_CHANGED,
  GRAB_NOTIFY,
  CHILD_NOTIFY,
  MNEMONIC_ACTIVATE,
  GRAB_FOCUS,
  FOCUS,
  MOVE_FOCUS,
  EVENT,
  EVENT_AFTER,
  BUTTON_PRESS_EVENT,
  BUTTON_RELEASE_EVENT,
  SCROLL_EVENT,
  MOTION_NOTIFY_EVENT,
  DELETE_EVENT,
  DESTROY_EVENT,
  EXPOSE_EVENT,
  KEY_PRESS_EVENT,
  KEY_RELEASE_EVENT,
  ENTER_NOTIFY_EVENT,
  LEAVE_NOTIFY_EVENT,
  CONFIGURE_EVENT,
  FOCUS_IN_EVENT,
  FOCUS_OUT_EVENT,
  MAP_EVENT,
  UNMAP_EVENT,
  PROPERTY_NOTIFY_EVENT,
  SELECTION_CLEAR_EVENT,
  SELECTION_REQUEST_EVENT,
  SELECTION_NOTIFY_EVENT,
  SELECTION_GET,
  SELECTION_RECEIVED,
  PROXIMITY_IN_EVENT,
  PROXIMITY_OUT_EVENT,
  DRAG_BEGIN,
  DRAG_END,
  DRAG_DATA_DELETE,
  DRAG_LEAVE,
  DRAG_MOTION,
  DRAG_DROP,
  DRAG_DATA_GET,
  DRAG_DATA_RECEIVED,
  CLIENT_EVENT,
  NO_EXPOSE_EVENT,
  VISIBILITY_NOTIFY_EVENT,
  WINDOW_STATE_EVENT,
  POPUP_MENU,
  SHOW_HELP,
  ACCEL_CLOSURES_CHANGED,
  SCREEN_CHANGED,
  CAN_ACTIVATE_ACCEL,
  GRAB_BROKEN,
  COMPOSITED_CHANGED,
  QUERY_TOOLTIP,
  KEYNAV_FAILED,
  DRAG_FAILED,
  DAMAGE_EVENT,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_NAME,
  PROP_PARENT,
  PROP_WIDTH_REQUEST,
  PROP_HEIGHT_REQUEST,
  PROP_VISIBLE,
  PROP_SENSITIVE,
  PROP_APP_PAINTABLE,
  PROP_CAN_FOCUS,
  PROP_HAS_FOCUS,
  PROP_IS_FOCUS,
  PROP_CAN_DEFAULT,
  PROP_HAS_DEFAULT,
  PROP_RECEIVES_DEFAULT,
  PROP_COMPOSITE_CHILD,
  PROP_STYLE,
  PROP_EVENTS,
  PROP_EXTENSION_EVENTS,
  PROP_NO_SHOW_ALL,
  PROP_HAS_TOOLTIP,
  PROP_TOOLTIP_MARKUP,
  PROP_TOOLTIP_TEXT,
  PROP_WINDOW,
  PROP_DOUBLE_BUFFERED
};

typedef	struct	_BtkStateData	 BtkStateData;

struct _BtkStateData
{
  BtkStateType  state;
  guint		state_restoration : 1;
  guint         parent_sensitive : 1;
  guint		use_forall : 1;
};

/* --- prototypes --- */
static void	btk_widget_class_init		(BtkWidgetClass     *klass);
static void	btk_widget_base_class_finalize	(BtkWidgetClass     *klass);
static void	btk_widget_init			(BtkWidget          *widget);
static void	btk_widget_set_property		 (GObject           *object,
						  guint              prop_id,
						  const GValue      *value,
						  GParamSpec        *pspec);
static void	btk_widget_get_property		 (GObject           *object,
						  guint              prop_id,
						  GValue            *value,
						  GParamSpec        *pspec);
static void	btk_widget_dispose		 (GObject	    *object);
static void	btk_widget_real_destroy		 (BtkObject	    *object);
static void	btk_widget_finalize		 (GObject	    *object);
static void	btk_widget_real_show		 (BtkWidget	    *widget);
static void	btk_widget_real_hide		 (BtkWidget	    *widget);
static void	btk_widget_real_map		 (BtkWidget	    *widget);
static void	btk_widget_real_unmap		 (BtkWidget	    *widget);
static void	btk_widget_real_realize		 (BtkWidget	    *widget);
static void	btk_widget_real_unrealize	 (BtkWidget	    *widget);
static void	btk_widget_real_size_request	 (BtkWidget	    *widget,
						  BtkRequisition    *requisition);
static void	btk_widget_real_size_allocate	 (BtkWidget	    *widget,
						  BtkAllocation	    *allocation);
static void	btk_widget_real_style_set        (BtkWidget         *widget,
                                                  BtkStyle          *previous_style);
static void	btk_widget_real_direction_changed(BtkWidget         *widget,
                                                  BtkTextDirection   previous_direction);

static void	btk_widget_real_grab_focus	 (BtkWidget         *focus_widget);
static gboolean btk_widget_real_query_tooltip    (BtkWidget         *widget,
						  gint               x,
						  gint               y,
						  gboolean           keyboard_tip,
						  BtkTooltip        *tooltip);
static gboolean btk_widget_real_show_help        (BtkWidget         *widget,
                                                  BtkWidgetHelpType  help_type);

static void	btk_widget_dispatch_child_properties_changed	(BtkWidget        *object,
								 guint             n_pspecs,
								 GParamSpec      **pspecs);
static gboolean		btk_widget_real_key_press_event   	(BtkWidget        *widget,
								 BdkEventKey      *event);
static gboolean		btk_widget_real_key_release_event 	(BtkWidget        *widget,
								 BdkEventKey      *event);
static gboolean		btk_widget_real_focus_in_event   	 (BtkWidget       *widget,
								  BdkEventFocus   *event);
static gboolean		btk_widget_real_focus_out_event   	(BtkWidget        *widget,
								 BdkEventFocus    *event);
static gboolean		btk_widget_real_focus			(BtkWidget        *widget,
								 BtkDirectionType  direction);
static void             btk_widget_real_move_focus              (BtkWidget        *widget,
                                                                 BtkDirectionType  direction);
static gboolean		btk_widget_real_keynav_failed		(BtkWidget        *widget,
								 BtkDirectionType  direction);
static BangoContext*	btk_widget_peek_bango_context		(BtkWidget	  *widget);
static void     	btk_widget_update_bango_context		(BtkWidget	  *widget);
static void		btk_widget_propagate_state		(BtkWidget	  *widget,
								 BtkStateData 	  *data);
static void             btk_widget_reset_rc_style               (BtkWidget        *widget);
static void		btk_widget_set_style_internal		(BtkWidget	  *widget,
								 BtkStyle	  *style,
								 gboolean	   initial_emission);
static gint		btk_widget_event_internal		(BtkWidget	  *widget,
								 BdkEvent	  *event);
static gboolean		btk_widget_real_mnemonic_activate	(BtkWidget	  *widget,
								 gboolean	   group_cycling);
static void		btk_widget_aux_info_destroy		(BtkWidgetAuxInfo *aux_info);
static BatkObject*	btk_widget_real_get_accessible		(BtkWidget	  *widget);
static void		btk_widget_accessible_interface_init	(BatkImplementorIface *iface);
static BatkObject*	btk_widget_ref_accessible		(BatkImplementor *implementor);
static void             btk_widget_invalidate_widget_windows    (BtkWidget        *widget,
								 BdkRebunnyion        *rebunnyion);
static BdkScreen *      btk_widget_get_screen_unchecked         (BtkWidget        *widget);
static void		btk_widget_queue_shallow_draw		(BtkWidget        *widget);
static gboolean         btk_widget_real_can_activate_accel      (BtkWidget *widget,
                                                                 guint      signal_id);

static void             btk_widget_real_set_has_tooltip         (BtkWidget *widget,
								 gboolean   has_tooltip,
								 gboolean   force);
static void             btk_widget_buildable_interface_init     (BtkBuildableIface *iface);
static void             btk_widget_buildable_set_name           (BtkBuildable     *buildable,
                                                                 const gchar      *name);
static const gchar *    btk_widget_buildable_get_name           (BtkBuildable     *buildable);
static GObject *        btk_widget_buildable_get_internal_child (BtkBuildable *buildable,
								 BtkBuilder   *builder,
								 const gchar  *childname);
static void             btk_widget_buildable_set_buildable_property (BtkBuildable     *buildable,
								     BtkBuilder       *builder,
								     const gchar      *name,
								     const GValue     *value);
static gboolean         btk_widget_buildable_custom_tag_start   (BtkBuildable     *buildable,
                                                                 BtkBuilder       *builder,
                                                                 GObject          *child,
                                                                 const gchar      *tagname,
                                                                 GMarkupParser    *parser,
                                                                 gpointer         *data);
static void             btk_widget_buildable_custom_finished    (BtkBuildable     *buildable,
                                                                 BtkBuilder       *builder,
                                                                 GObject          *child,
                                                                 const gchar      *tagname,
                                                                 gpointer          data);
static void             btk_widget_buildable_parser_finished    (BtkBuildable     *buildable,
                                                                 BtkBuilder       *builder);

static void             btk_widget_queue_tooltip_query          (BtkWidget *widget);
     
static void btk_widget_set_usize_internal (BtkWidget *widget,
					   gint       width,
					   gint       height);
static void btk_widget_get_draw_rectangle (BtkWidget    *widget,
					   BdkRectangle *rect);


/* --- variables --- */
static gpointer         btk_widget_parent_class = NULL;
static guint            widget_signals[LAST_SIGNAL] = { 0 };
static BtkStyle        *btk_default_style = NULL;
static GSList          *colormap_stack = NULL;
static guint            composite_child_stack = 0;
static BtkTextDirection btk_default_direction = BTK_TEXT_DIR_LTR;
static GParamSpecPool  *style_property_spec_pool = NULL;

static GQuark		quark_property_parser = 0;
static GQuark		quark_aux_info = 0;
static GQuark		quark_accel_path = 0;
static GQuark		quark_accel_closures = 0;
static GQuark		quark_event_mask = 0;
static GQuark		quark_extension_event_mode = 0;
static GQuark		quark_parent_window = 0;
static GQuark		quark_pointer_window = 0;
static GQuark		quark_shape_info = 0;
static GQuark		quark_input_shape_info = 0;
static GQuark		quark_colormap = 0;
static GQuark		quark_bango_context = 0;
static GQuark		quark_rc_style = 0;
static GQuark		quark_accessible_object = 0;
static GQuark		quark_mnemonic_labels = 0;
static GQuark		quark_tooltip_markup = 0;
static GQuark		quark_has_tooltip = 0;
static GQuark		quark_tooltip_window = 0;
GParamSpecPool         *_btk_widget_child_property_pool = NULL;
GObjectNotifyContext   *_btk_widget_child_property_notify_context = NULL;

/* --- functions --- */
GType
btk_widget_get_type (void)
{
  static GType widget_type = 0;

  if (B_UNLIKELY (widget_type == 0))
    {
      const GTypeInfo widget_info =
      {
	sizeof (BtkWidgetClass),
	NULL,		/* base_init */
	(GBaseFinalizeFunc) btk_widget_base_class_finalize,
	(GClassInitFunc) btk_widget_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_init */
	sizeof (BtkWidget),
	0,		/* n_preallocs */
	(GInstanceInitFunc) btk_widget_init,
	NULL,		/* value_table */
      };

      const GInterfaceInfo accessibility_info =
      {
	(GInterfaceInitFunc) btk_widget_accessible_interface_init,
	(GInterfaceFinalizeFunc) NULL,
	NULL /* interface data */
      };

      const GInterfaceInfo buildable_info =
      {
	(GInterfaceInitFunc) btk_widget_buildable_interface_init,
	(GInterfaceFinalizeFunc) NULL,
	NULL /* interface data */
      };

      widget_type = g_type_register_static (BTK_TYPE_OBJECT, "BtkWidget",
                                           &widget_info, G_TYPE_FLAG_ABSTRACT);

      g_type_add_interface_static (widget_type, BATK_TYPE_IMPLEMENTOR,
                                   &accessibility_info) ;
      g_type_add_interface_static (widget_type, BTK_TYPE_BUILDABLE,
                                   &buildable_info) ;

    }

  return widget_type;
}

static void
child_property_notify_dispatcher (GObject     *object,
				  guint        n_pspecs,
				  GParamSpec **pspecs)
{
  BTK_WIDGET_GET_CLASS (object)->dispatch_child_properties_changed (BTK_WIDGET (object), n_pspecs, pspecs);
}

static void
btk_widget_class_init (BtkWidgetClass *klass)
{
  static GObjectNotifyContext cpn_context = { 0, NULL, NULL };
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkObjectClass *object_class = BTK_OBJECT_CLASS (klass);
  BtkBindingSet *binding_set;

  btk_widget_parent_class = g_type_class_peek_parent (klass);

  quark_property_parser = g_quark_from_static_string ("btk-rc-property-parser");
  quark_aux_info = g_quark_from_static_string ("btk-aux-info");
  quark_accel_path = g_quark_from_static_string ("btk-accel-path");
  quark_accel_closures = g_quark_from_static_string ("btk-accel-closures");
  quark_event_mask = g_quark_from_static_string ("btk-event-mask");
  quark_extension_event_mode = g_quark_from_static_string ("btk-extension-event-mode");
  quark_parent_window = g_quark_from_static_string ("btk-parent-window");
  quark_pointer_window = g_quark_from_static_string ("btk-pointer-window");
  quark_shape_info = g_quark_from_static_string ("btk-shape-info");
  quark_input_shape_info = g_quark_from_static_string ("btk-input-shape-info");
  quark_colormap = g_quark_from_static_string ("btk-colormap");
  quark_bango_context = g_quark_from_static_string ("btk-bango-context");
  quark_rc_style = g_quark_from_static_string ("btk-rc-style");
  quark_accessible_object = g_quark_from_static_string ("btk-accessible-object");
  quark_mnemonic_labels = g_quark_from_static_string ("btk-mnemonic-labels");
  quark_tooltip_markup = g_quark_from_static_string ("btk-tooltip-markup");
  quark_has_tooltip = g_quark_from_static_string ("btk-has-tooltip");
  quark_tooltip_window = g_quark_from_static_string ("btk-tooltip-window");

  style_property_spec_pool = g_param_spec_pool_new (FALSE);
  _btk_widget_child_property_pool = g_param_spec_pool_new (TRUE);
  cpn_context.quark_notify_queue = g_quark_from_static_string ("BtkWidget-child-property-notify-queue");
  cpn_context.dispatcher = child_property_notify_dispatcher;
  _btk_widget_child_property_notify_context = &cpn_context;

  bobject_class->dispose = btk_widget_dispose;
  bobject_class->finalize = btk_widget_finalize;
  bobject_class->set_property = btk_widget_set_property;
  bobject_class->get_property = btk_widget_get_property;

  object_class->destroy = btk_widget_real_destroy;
  
  klass->activate_signal = 0;
  klass->set_scroll_adjustments_signal = 0;
  klass->dispatch_child_properties_changed = btk_widget_dispatch_child_properties_changed;
  klass->show = btk_widget_real_show;
  klass->show_all = btk_widget_show;
  klass->hide = btk_widget_real_hide;
  klass->hide_all = btk_widget_hide;
  klass->map = btk_widget_real_map;
  klass->unmap = btk_widget_real_unmap;
  klass->realize = btk_widget_real_realize;
  klass->unrealize = btk_widget_real_unrealize;
  klass->size_request = btk_widget_real_size_request;
  klass->size_allocate = btk_widget_real_size_allocate;
  klass->state_changed = NULL;
  klass->parent_set = NULL;
  klass->hierarchy_changed = NULL;
  klass->style_set = btk_widget_real_style_set;
  klass->direction_changed = btk_widget_real_direction_changed;
  klass->grab_notify = NULL;
  klass->child_notify = NULL;
  klass->mnemonic_activate = btk_widget_real_mnemonic_activate;
  klass->grab_focus = btk_widget_real_grab_focus;
  klass->focus = btk_widget_real_focus;
  klass->event = NULL;
  klass->button_press_event = NULL;
  klass->button_release_event = NULL;
  klass->motion_notify_event = NULL;
  klass->delete_event = NULL;
  klass->destroy_event = NULL;
  klass->expose_event = NULL;
  klass->key_press_event = btk_widget_real_key_press_event;
  klass->key_release_event = btk_widget_real_key_release_event;
  klass->enter_notify_event = NULL;
  klass->leave_notify_event = NULL;
  klass->configure_event = NULL;
  klass->focus_in_event = btk_widget_real_focus_in_event;
  klass->focus_out_event = btk_widget_real_focus_out_event;
  klass->map_event = NULL;
  klass->unmap_event = NULL;
  klass->window_state_event = NULL;
  klass->property_notify_event = _btk_selection_property_notify;
  klass->selection_clear_event = btk_selection_clear;
  klass->selection_request_event = _btk_selection_request;
  klass->selection_notify_event = _btk_selection_notify;
  klass->selection_received = NULL;
  klass->proximity_in_event = NULL;
  klass->proximity_out_event = NULL;
  klass->drag_begin = NULL;
  klass->drag_end = NULL;
  klass->drag_data_delete = NULL;
  klass->drag_leave = NULL;
  klass->drag_motion = NULL;
  klass->drag_drop = NULL;
  klass->drag_data_received = NULL;
  klass->screen_changed = NULL;
  klass->can_activate_accel = btk_widget_real_can_activate_accel;
  klass->grab_broken_event = NULL;
  klass->query_tooltip = btk_widget_real_query_tooltip;

  klass->show_help = btk_widget_real_show_help;
  
  /* Accessibility support */
  klass->get_accessible = btk_widget_real_get_accessible;

  klass->no_expose_event = NULL;

  g_object_class_install_property (bobject_class,
				   PROP_NAME,
				   g_param_spec_string ("name",
 							P_("Widget name"),
							P_("The name of the widget"),
							NULL,
							BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_PARENT,
				   g_param_spec_object ("parent",
							P_("Parent widget"), 
							P_("The parent widget of this widget. Must be a Container widget"),
							BTK_TYPE_CONTAINER,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_WIDTH_REQUEST,
				   g_param_spec_int ("width-request",
 						     P_("Width request"),
 						     P_("Override for width request of the widget, or -1 if natural request should be used"),
 						     -1,
 						     G_MAXINT,
 						     -1,
 						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_HEIGHT_REQUEST,
				   g_param_spec_int ("height-request",
 						     P_("Height request"),
 						     P_("Override for height request of the widget, or -1 if natural request should be used"),
 						     -1,
 						     G_MAXINT,
 						     -1,
 						     BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
 							 P_("Visible"),
 							 P_("Whether the widget is visible"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
 							 P_("Sensitive"),
 							 P_("Whether the widget responds to input"),
 							 TRUE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_APP_PAINTABLE,
				   g_param_spec_boolean ("app-paintable",
 							 P_("Application paintable"),
 							 P_("Whether the application will paint directly on the widget"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_CAN_FOCUS,
				   g_param_spec_boolean ("can-focus",
 							 P_("Can focus"),
 							 P_("Whether the widget can accept the input focus"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_HAS_FOCUS,
				   g_param_spec_boolean ("has-focus",
 							 P_("Has focus"),
 							 P_("Whether the widget has the input focus"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_IS_FOCUS,
				   g_param_spec_boolean ("is-focus",
 							 P_("Is focus"),
 							 P_("Whether the widget is the focus widget within the toplevel"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_CAN_DEFAULT,
				   g_param_spec_boolean ("can-default",
 							 P_("Can default"),
 							 P_("Whether the widget can be the default widget"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_HAS_DEFAULT,
				   g_param_spec_boolean ("has-default",
 							 P_("Has default"),
 							 P_("Whether the widget is the default widget"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_RECEIVES_DEFAULT,
				   g_param_spec_boolean ("receives-default",
 							 P_("Receives default"),
 							 P_("If TRUE, the widget will receive the default action when it is focused"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_COMPOSITE_CHILD,
				   g_param_spec_boolean ("composite-child",
 							 P_("Composite child"),
 							 P_("Whether the widget is part of a composite widget"),
 							 FALSE,
 							 BTK_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
				   PROP_STYLE,
				   g_param_spec_object ("style",
 							P_("Style"),
 							P_("The style of the widget, which contains information about how it will look (colors etc)"),
 							BTK_TYPE_STYLE,
 							BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_EVENTS,
				   g_param_spec_flags ("events",
 						       P_("Events"),
 						       P_("The event mask that decides what kind of BdkEvents this widget gets"),
 						       BDK_TYPE_EVENT_MASK,
 						       BDK_STRUCTURE_MASK,
 						       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_EXTENSION_EVENTS,
				   g_param_spec_enum ("extension-events",
 						      P_("Extension events"),
 						      P_("The mask that decides what kind of extension events this widget gets"),
 						      BDK_TYPE_EXTENSION_MODE,
 						      BDK_EXTENSION_EVENTS_NONE,
 						      BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_NO_SHOW_ALL,
				   g_param_spec_boolean ("no-show-all",
 							 P_("No show all"),
 							 P_("Whether btk_widget_show_all() should not affect this widget"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));

/**
 * BtkWidget:has-tooltip:
 *
 * Enables or disables the emission of #BtkWidget::query-tooltip on @widget.  
 * A value of %TRUE indicates that @widget can have a tooltip, in this case
 * the widget will be queried using #BtkWidget::query-tooltip to determine
 * whether it will provide a tooltip or not.
 *
 * Note that setting this property to %TRUE for the first time will change
 * the event masks of the BdkWindows of this widget to include leave-notify
 * and motion-notify events.  This cannot and will not be undone when the
 * property is set to %FALSE again.
 *
 * Since: 2.12
 */
  g_object_class_install_property (bobject_class,
				   PROP_HAS_TOOLTIP,
				   g_param_spec_boolean ("has-tooltip",
 							 P_("Has tooltip"),
 							 P_("Whether this widget has a tooltip"),
 							 FALSE,
 							 BTK_PARAM_READWRITE));
  /**
   * BtkWidget:tooltip-text:
   *
   * Sets the text of tooltip to be the given string.
   *
   * Also see btk_tooltip_set_text().
   *
   * This is a convenience property which will take care of getting the
   * tooltip shown if the given string is not %NULL: #BtkWidget:has-tooltip
   * will automatically be set to %TRUE and there will be taken care of
   * #BtkWidget::query-tooltip in the default signal handler.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
                                   PROP_TOOLTIP_TEXT,
                                   g_param_spec_string ("tooltip-text",
                                                        P_("Tooltip Text"),
                                                        P_("The contents of the tooltip for this widget"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  /**
   * BtkWidget:tooltip-markup:
   *
   * Sets the text of tooltip to be the given string, which is marked up
   * with the <link linkend="BangoMarkupFormat">Bango text markup language</link>.
   * Also see btk_tooltip_set_markup().
   *
   * This is a convenience property which will take care of getting the
   * tooltip shown if the given string is not %NULL: #BtkWidget:has-tooltip
   * will automatically be set to %TRUE and there will be taken care of
   * #BtkWidget::query-tooltip in the default signal handler.
   *
   * Since: 2.12
   */
  g_object_class_install_property (bobject_class,
				   PROP_TOOLTIP_MARKUP,
				   g_param_spec_string ("tooltip-markup",
 							P_("Tooltip markup"),
							P_("The contents of the tooltip for this widget"),
							NULL,
							BTK_PARAM_READWRITE));

  /**
   * BtkWidget:window:
   *
   * The widget's window if it is realized, %NULL otherwise.
   *
   * Since: 2.14
   */
  g_object_class_install_property (bobject_class,
				   PROP_WINDOW,
				   g_param_spec_object ("window",
 							P_("Window"),
							P_("The widget's window if it is realized"),
							BDK_TYPE_WINDOW,
							BTK_PARAM_READABLE));

  /**
   * BtkWidget:double-buffered
   *
   * Whether or not the widget is double buffered.
   *
   * Since: 2.18
   */
  g_object_class_install_property (bobject_class,
                                   PROP_DOUBLE_BUFFERED,
                                   g_param_spec_boolean ("double-buffered",
                                                         P_("Double Buffered"),
                                                         P_("Whether or not the widget is double buffered"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));

  /**
   * BtkWidget::show:
   * @widget: the object which received the signal.
   */
  widget_signals[SHOW] =
    g_signal_new (I_("show"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, show),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::hide:
   * @widget: the object which received the signal.
   */
  widget_signals[HIDE] =
    g_signal_new (I_("hide"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, hide),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::map:
   * @widget: the object which received the signal.
   */
  widget_signals[MAP] =
    g_signal_new (I_("map"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, map),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::unmap:
   * @widget: the object which received the signal.
   */
  widget_signals[UNMAP] =
    g_signal_new (I_("unmap"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, unmap),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::realize:
   * @widget: the object which received the signal.
   */
  widget_signals[REALIZE] =
    g_signal_new (I_("realize"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, realize),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::unrealize:
   * @widget: the object which received the signal.
   */
  widget_signals[UNREALIZE] =
    g_signal_new (I_("unrealize"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, unrealize),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::size-request:
   * @widget: the object which received the signal.
   * @requisition:
   */
  widget_signals[SIZE_REQUEST] =
    g_signal_new (I_("size-request"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, size_request),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_REQUISITION | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::size-allocate:
   * @widget: the object which received the signal.
   * @allocation:
   */
  widget_signals[SIZE_ALLOCATE] = 
    g_signal_new (I_("size-allocate"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, size_allocate),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED,
		  G_TYPE_NONE, 1,
		  BDK_TYPE_RECTANGLE | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::state-changed:
   * @widget: the object which received the signal.
   * @state: the previous state
   *
   * The ::state-changed signal is emitted when the widget state changes.
   * See btk_widget_get_state().
   */
  widget_signals[STATE_CHANGED] =
    g_signal_new (I_("state-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, state_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_STATE_TYPE);

  /**
   * BtkWidget::parent-set:
   * @widget: the object on which the signal is emitted
   * @old_parent: (allow-none): the previous parent, or %NULL if the widget
   *   just got its initial parent.
   *
   * The ::parent-set signal is emitted when a new parent 
   * has been set on a widget. 
   */
  widget_signals[PARENT_SET] =
    g_signal_new (I_("parent-set"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, parent_set),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);

  /**
   * BtkWidget::hierarchy-changed:
   * @widget: the object on which the signal is emitted
   * @previous_toplevel: (allow-none): the previous toplevel ancestor, or %NULL
   *   if the widget was previously unanchored
   *
   * The ::hierarchy-changed signal is emitted when the
   * anchored state of a widget changes. A widget is
   * <firstterm>anchored</firstterm> when its toplevel
   * ancestor is a #BtkWindow. This signal is emitted when
   * a widget changes from un-anchored to anchored or vice-versa.
   */
  widget_signals[HIERARCHY_CHANGED] =
    g_signal_new (I_("hierarchy-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, hierarchy_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_WIDGET);

  /**
   * BtkWidget::style-set:
   * @widget: the object on which the signal is emitted
   * @previous_style: (allow-none): the previous style, or %NULL if the widget
   *   just got its initial style 
   *
   * The ::style-set signal is emitted when a new style has been set 
   * on a widget. Note that style-modifying functions like 
   * btk_widget_modify_base() also cause this signal to be emitted.
   */
  widget_signals[STYLE_SET] =
    g_signal_new (I_("style-set"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, style_set),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_STYLE);
/**
 * BtkWidget::direction-changed:
 * @widget: the object on which the signal is emitted
 * @previous_direction: the previous text direction of @widget
 *
 * The ::direction-changed signal is emitted when the text direction
 * of a widget changes.
 */
  widget_signals[DIRECTION_CHANGED] =
    g_signal_new (I_("direction-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkWidgetClass, direction_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__ENUM,
		  G_TYPE_NONE, 1,
		  BTK_TYPE_TEXT_DIRECTION);

  /**
   * BtkWidget::grab-notify:
   * @widget: the object which received the signal
   * @was_grabbed: %FALSE if the widget becomes shadowed, %TRUE
   *               if it becomes unshadowed
   *
   * The ::grab-notify signal is emitted when a widget becomes
   * shadowed by a BTK+ grab (not a pointer or keyboard grab) on 
   * another widget, or when it becomes unshadowed due to a grab 
   * being removed.
   * 
   * A widget is shadowed by a btk_grab_add() when the topmost 
   * grab widget in the grab stack of its window group is not 
   * its ancestor.
   */
  widget_signals[GRAB_NOTIFY] =
    g_signal_new (I_("grab-notify"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BtkWidgetClass, grab_notify),
		  NULL, NULL,
		  _btk_marshal_VOID__BOOLEAN,
		  G_TYPE_NONE, 1,
		  G_TYPE_BOOLEAN);

/**
 * BtkWidget::child-notify:
 * @widget: the object which received the signal
 * @pspec: the #GParamSpec of the changed child property
 *
 * The ::child-notify signal is emitted for each 
 * <link linkend="child-properties">child property</link>  that has
 * changed on an object. The signal's detail holds the property name. 
 */
  widget_signals[CHILD_NOTIFY] =
    g_signal_new (I_("child-notify"),
		   G_TYPE_FROM_CLASS (bobject_class),
		   G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED | G_SIGNAL_NO_HOOKS,
		   G_STRUCT_OFFSET (BtkWidgetClass, child_notify),
		   NULL, NULL,
		   g_cclosure_marshal_VOID__PARAM,
		   G_TYPE_NONE, 1,
		   G_TYPE_PARAM);

  /**
   * BtkWidget::mnemonic-activate:
   * @widget: the object which received the signal.
   * @arg1:
   */
  widget_signals[MNEMONIC_ACTIVATE] =
    g_signal_new (I_("mnemonic-activate"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, mnemonic_activate),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOOLEAN,
		  G_TYPE_BOOLEAN, 1,
		  G_TYPE_BOOLEAN);

  /**
   * BtkWidget::grab-focus:
   * @widget: the object which received the signal.
   */
  widget_signals[GRAB_FOCUS] =
    g_signal_new (I_("grab-focus"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkWidgetClass, grab_focus),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::focus:
   * @widget: the object which received the signal.
   * @direction:
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   */
  widget_signals[FOCUS] =
    g_signal_new (I_("focus"),
		  G_TYPE_FROM_CLASS (object_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, focus),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__ENUM,
		  G_TYPE_BOOLEAN, 1,
		  BTK_TYPE_DIRECTION_TYPE);

  /**
   * BtkWidget::move-focus:
   * @widget: the object which received the signal.
   * @direction:
   */
  widget_signals[MOVE_FOCUS] =
    g_signal_new_class_handler (I_("move-focus"),
                                G_TYPE_FROM_CLASS (object_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (btk_widget_real_move_focus),
                                NULL, NULL,
                                _btk_marshal_VOID__ENUM,
                                G_TYPE_NONE,
                                1,
                                BTK_TYPE_DIRECTION_TYPE);
  /**
   * BtkWidget::event:
   * @widget: the object which received the signal.
   * @event: the #BdkEvent which triggered this signal
   *
   * The BTK+ main loop will emit three signals for each BDK event delivered
   * to a widget: one generic ::event signal, another, more specific,
   * signal that matches the type of event delivered (e.g. 
   * #BtkWidget::key-press-event) and finally a generic 
   * #BtkWidget::event-after signal.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event 
   * and to cancel the emission of the second specific ::event signal.
   *   %FALSE to propagate the event further and to allow the emission of 
   *   the second signal. The ::event-after signal is emitted regardless of
   *   the return value.
   */
  widget_signals[EVENT] =
    g_signal_new (I_("event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::event-after:
   * @widget: the object which received the signal.
   * @event: the #BdkEvent which triggered this signal
   *
   * After the emission of the #BtkWidget::event signal and (optionally) 
   * the second more specific signal, ::event-after will be emitted 
   * regardless of the previous two signals handlers return values.
   *
   */
  widget_signals[EVENT_AFTER] =
    g_signal_new (I_("event-after"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  0,
		  0,
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED,
		  G_TYPE_NONE, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::button-press-event:
   * @widget: the object which received the signal.
   * @event: (type Bdk.EventButton): the #BdkEventButton which triggered
   *   this signal.
   *
   * The ::button-press-event signal will be emitted when a button
   * (typically from a mouse) is pressed.
   *
   * To receive this signal, the #BdkWindow associated to the 
   * widget needs to enable the #BDK_BUTTON_PRESS_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[BUTTON_PRESS_EVENT] =
    g_signal_new (I_("button-press-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, button_press_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::button-release-event:
   * @widget: the object which received the signal.
   * @event: (type Bdk.EventButton): the #BdkEventButton which triggered
   *   this signal.
   *
   * The ::button-release-event signal will be emitted when a button
   * (typically from a mouse) is released.
   *
   * To receive this signal, the #BdkWindow associated to the 
   * widget needs to enable the #BDK_BUTTON_RELEASE_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[BUTTON_RELEASE_EVENT] =
    g_signal_new (I_("button-release-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, button_release_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::scroll-event:
   * @widget: the object which received the signal.
   * @event: (type Bdk.EventScroll): the #BdkEventScroll which triggered
   *   this signal.
   *
   * The ::scroll-event signal is emitted when a button in the 4 to 7
   * range is pressed. Wheel mice are usually configured to generate 
   * button press events for buttons 4 and 5 when the wheel is turned.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_BUTTON_PRESS_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[SCROLL_EVENT] =
    g_signal_new (I_("scroll-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, scroll_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * BtkWidget::motion-notify-event:
   * @widget: the object which received the signal.
   * @event: (type Bdk.EventMotion): the #BdkEventMotion which triggered
   *   this signal.
   *
   * The ::motion-notify-event signal is emitted when the pointer moves 
   * over the widget's #BdkWindow.
   *
   * To receive this signal, the #BdkWindow associated to the widget 
   * needs to enable the #BDK_POINTER_MOTION_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[MOTION_NOTIFY_EVENT] =
    g_signal_new (I_("motion-notify-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, motion_notify_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::composited-changed:
   * @widget: the object on which the signal is emitted
   *
   * The ::composited-changed signal is emitted when the composited
   * status of @widget<!-- -->s screen changes. 
   * See bdk_screen_is_composited().
   */
  widget_signals[COMPOSITED_CHANGED] =
    g_signal_new (I_("composited-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkWidgetClass, composited_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::keynav-failed:
   * @widget: the object which received the signal
   * @direction: the direction of movement
   *
   * Gets emitted if keyboard navigation fails. 
   * See btk_widget_keynav_failed() for details.
   *
   * Returns: %TRUE if stopping keyboard navigation is fine, %FALSE
   *          if the emitting widget should try to handle the keyboard
   *          navigation attempt in its parent container(s).
   *
   * Since: 2.12
   **/
  widget_signals[KEYNAV_FAILED] =
    g_signal_new_class_handler (I_("keynav-failed"),
                                G_TYPE_FROM_CLASS (bobject_class),
                                G_SIGNAL_RUN_LAST,
                                G_CALLBACK (btk_widget_real_keynav_failed),
                                _btk_boolean_handled_accumulator, NULL,
                                _btk_marshal_BOOLEAN__ENUM,
                                G_TYPE_BOOLEAN, 1,
                                BTK_TYPE_DIRECTION_TYPE);

  /**
   * BtkWidget::delete-event:
   * @widget: the object which received the signal
   * @event: the event which triggered this signal
   *
   * The ::delete-event signal is emitted if a user requests that
   * a toplevel window is closed. The default handler for this signal
   * destroys the window. Connecting btk_widget_hide_on_delete() to
   * this signal will cause the window to be hidden instead, so that
   * it can later be shown again without reconstructing it.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[DELETE_EVENT] =
    g_signal_new (I_("delete-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, delete_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::destroy-event:
   * @widget: the object which received the signal.
   * @event: the event which triggered this signal
   *
   * The ::destroy-event signal is emitted when a #BdkWindow is destroyed.
   * You rarely get this signal, because most widgets disconnect themselves 
   * from their window before they destroy it, so no widget owns the 
   * window at destroy time.
   * 
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_STRUCTURE_MASK mask. BDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[DESTROY_EVENT] =
    g_signal_new (I_("destroy-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, destroy_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::expose-event:
   * @widget: the object which received the signal.
   * @event: (type Bdk.EventExpose): the #BdkEventExpose which triggered
   *   this signal.
   *
   * The ::expose-event signal is emitted when an area of a previously
   * obscured #BdkWindow is made visible and needs to be redrawn.
   * #BTK_NO_WINDOW widgets will get a synthesized event from their parent
   * widget.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_EXPOSURE_MASK mask.
   *
   * Note that the ::expose-event signal has been replaced by a ::draw
   * signal in BTK+ 3. The <link linkend="http://library.bunny.org/devel/btk3/3.0/btk-migrating-2-to-3.html">BTK+ 3 migration guide</link>
   * for hints on how to port from ::expose-event to ::draw.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  widget_signals[EXPOSE_EVENT] =
    g_signal_new (I_("expose-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, expose_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::key-press-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventKey): the #BdkEventKey which triggered this signal.
   *
   * The ::key-press-event signal is emitted when a key is pressed.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_KEY_PRESS_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[KEY_PRESS_EVENT] =
    g_signal_new (I_("key-press-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, key_press_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::key-release-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventKey): the #BdkEventKey which triggered this signal.
   *
   * The ::key-release-event signal is emitted when a key is pressed.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_KEY_RELEASE_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[KEY_RELEASE_EVENT] =
    g_signal_new (I_("key-release-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, key_release_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::enter-notify-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventCrossing): the #BdkEventCrossing which triggered
   *   this signal.
   *
   * The ::enter-notify-event will be emitted when the pointer enters
   * the @widget's window.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_ENTER_NOTIFY_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[ENTER_NOTIFY_EVENT] =
    g_signal_new (I_("enter-notify-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, enter_notify_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::leave-notify-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventCrossing): the #BdkEventCrossing which triggered
   *   this signal.
   *
   * The ::leave-notify-event will be emitted when the pointer leaves
   * the @widget's window.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_LEAVE_NOTIFY_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[LEAVE_NOTIFY_EVENT] =
    g_signal_new (I_("leave-notify-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, leave_notify_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::configure-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventConfigure): the #BdkEventConfigure which triggered
   *   this signal.
   *
   * The ::configure-event signal will be emitted when the size, position or
   * stacking of the @widget's window has changed.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_STRUCTURE_MASK mask. BDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[CONFIGURE_EVENT] =
    g_signal_new (I_("configure-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, configure_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::focus-in-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventFocus): the #BdkEventFocus which triggered
   *   this signal.
   *
   * The ::focus-in-event signal will be emitted when the keyboard focus
   * enters the @widget's window.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_FOCUS_CHANGE_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[FOCUS_IN_EVENT] =
    g_signal_new (I_("focus-in-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, focus_in_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::focus-out-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventFocus): the #BdkEventFocus which triggered this
   *   signal.
   *
   * The ::focus-out-event signal will be emitted when the keyboard focus
   * leaves the @widget's window.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_FOCUS_CHANGE_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[FOCUS_OUT_EVENT] =
    g_signal_new (I_("focus-out-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, focus_out_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::map-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventAny): the #BdkEventAny which triggered this signal.
   *
   * The ::map-event signal will be emitted when the @widget's window is
   * mapped. A window is mapped when it becomes visible on the screen.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_STRUCTURE_MASK mask. BDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[MAP_EVENT] =
    g_signal_new (I_("map-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, map_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::unmap-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventAny): the #BdkEventAny which triggered this signal
   *
   * The ::unmap-event signal will be emitted when the @widget's window is
   * unmapped. A window is unmapped when it becomes invisible on the screen.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_STRUCTURE_MASK mask. BDK will enable this mask
   * automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[UNMAP_EVENT] =
    g_signal_new (I_("unmap-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, unmap_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::property-notify-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventProperty): the #BdkEventProperty which triggered
   *   this signal.
   *
   * The ::property-notify-event signal will be emitted when a property on
   * the @widget's window has been changed or deleted.
   *
   * To receive this signal, the #BdkWindow associated to the widget needs
   * to enable the #BDK_PROPERTY_CHANGE_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[PROPERTY_NOTIFY_EVENT] =
    g_signal_new (I_("property-notify-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, property_notify_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::selection-clear-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventSelection): the #BdkEventSelection which triggered
   *   this signal.
   *
   * The ::selection-clear-event signal will be emitted when the
   * the @widget's window has lost ownership of a selection.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[SELECTION_CLEAR_EVENT] =
    g_signal_new (I_("selection-clear-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, selection_clear_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::selection-request-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventSelection): the #BdkEventSelection which triggered
   *   this signal.
   *
   * The ::selection-request-event signal will be emitted when
   * another client requests ownership of the selection owned by
   * the @widget's window.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[SELECTION_REQUEST_EVENT] =
    g_signal_new (I_("selection-request-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, selection_request_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::selection-notify-event:
   * @widget: the object which received the signal.
   * @event:
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   */
  widget_signals[SELECTION_NOTIFY_EVENT] =
    g_signal_new (I_("selection-notify-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, selection_notify_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::selection-received:
   * @widget: the object which received the signal.
   * @data:
   * @time:
   */
  widget_signals[SELECTION_RECEIVED] =
    g_signal_new (I_("selection-received"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, selection_received),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED_UINT,
		  G_TYPE_NONE, 2,
		  BTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT);

  /**
   * BtkWidget::selection-get:
   * @widget: the object which received the signal.
   * @data:
   * @info:
   * @time:
   */
  widget_signals[SELECTION_GET] =
    g_signal_new (I_("selection-get"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, selection_get),
		  NULL, NULL,
		  _btk_marshal_VOID__BOXED_UINT_UINT,
		  G_TYPE_NONE, 3,
		  BTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT,
		  G_TYPE_UINT);

  /**
   * BtkWidget::proximity-in-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventProximity): the #BdkEventProximity which triggered
   *   this signal.
   *
   * To receive this signal the #BdkWindow associated to the widget needs
   * to enable the #BDK_PROXIMITY_IN_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[PROXIMITY_IN_EVENT] =
    g_signal_new (I_("proximity-in-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, proximity_in_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::proximity-out-event
   * @widget: the object which received the signal
   * @event: (type Bdk.EventProximity): the #BdkEventProximity which triggered
   *   this signal.
   *
   * To receive this signal the #BdkWindow associated to the widget needs
   * to enable the #BDK_PROXIMITY_OUT_MASK mask.
   *
   * This signal will be sent to the grab widget if there is one.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[PROXIMITY_OUT_EVENT] =
    g_signal_new (I_("proximity-out-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, proximity_out_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::drag-leave:
   * @widget: the object which received the signal.
   * @drag_context: the drag context
   * @time: the timestamp of the motion event
   *
   * The ::drag-leave signal is emitted on the drop site when the cursor 
   * leaves the widget. A typical reason to connect to this signal is to 
   * undo things done in #BtkWidget::drag-motion, e.g. undo highlighting 
   * with btk_drag_unhighlight()
   */
  widget_signals[DRAG_LEAVE] =
    g_signal_new (I_("drag-leave"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_leave),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_UINT,
		  G_TYPE_NONE, 2,
		  BDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_UINT);

  /**
   * BtkWidget::drag-begin:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   *
   * The ::drag-begin signal is emitted on the drag source when a drag is 
   * started. A typical reason to connect to this signal is to set up a 
   * custom drag icon with btk_drag_source_set_icon().
   *
   * Note that some widgets set up a drag icon in the default handler of
   * this signal, so you may have to use g_signal_connect_after() to
   * override what the default handler did.
   */
  widget_signals[DRAG_BEGIN] =
    g_signal_new (I_("drag-begin"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_begin),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BDK_TYPE_DRAG_CONTEXT);

  /**
   * BtkWidget::drag-end:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   *
   * The ::drag-end signal is emitted on the drag source when a drag is 
   * finished.  A typical reason to connect to this signal is to undo 
   * things done in #BtkWidget::drag-begin.
   */
  widget_signals[DRAG_END] =
    g_signal_new (I_("drag-end"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_end),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BDK_TYPE_DRAG_CONTEXT);

  /**
   * BtkWidget::drag-data-delete:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   *
   * The ::drag-data-delete signal is emitted on the drag source when a drag 
   * with the action %BDK_ACTION_MOVE is successfully completed. The signal 
   * handler is responsible for deleting the data that has been dropped. What 
   * "delete" means depends on the context of the drag operation. 
   */
  widget_signals[DRAG_DATA_DELETE] =
    g_signal_new (I_("drag-data-delete"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_data_delete),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BDK_TYPE_DRAG_CONTEXT);

  /**
   * BtkWidget::drag-failed:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   * @result: the result of the drag operation
   *
   * The ::drag-failed signal is emitted on the drag source when a drag has
   * failed. The signal handler may hook custom code to handle a failed DND
   * operation based on the type of error, it returns %TRUE is the failure has
   * been already handled (not showing the default "drag operation failed"
   * animation), otherwise it returns %FALSE.
   *
   * Return value: %TRUE if the failed drag operation has been already handled.
   *
   * Since: 2.12
   */
  widget_signals[DRAG_FAILED] =
    g_signal_new (I_("drag-failed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  0, _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__OBJECT_ENUM,
		  G_TYPE_BOOLEAN, 2,
		  BDK_TYPE_DRAG_CONTEXT,
		  BTK_TYPE_DRAG_RESULT);

  /**
   * BtkWidget::drag-motion:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   * @x: the x coordinate of the current cursor position
   * @y: the y coordinate of the current cursor position
   * @time: the timestamp of the motion event
   * @returns: whether the cursor position is in a drop zone
   *
   * The drag-motion signal is emitted on the drop site when the user
   * moves the cursor over the widget during a drag. The signal handler
   * must determine whether the cursor position is in a drop zone or not.
   * If it is not in a drop zone, it returns %FALSE and no further processing
   * is necessary. Otherwise, the handler returns %TRUE. In this case, the
   * handler is responsible for providing the necessary information for
   * displaying feedback to the user, by calling bdk_drag_status().
   *
   * If the decision whether the drop will be accepted or rejected can't be
   * made based solely on the cursor position and the type of the data, the
   * handler may inspect the dragged data by calling btk_drag_get_data() and
   * defer the bdk_drag_status() call to the #BtkWidget::drag-data-received
   * handler. Note that you cannot not pass #BTK_DEST_DEFAULT_DROP,
   * #BTK_DEST_DEFAULT_MOTION or #BTK_DEST_DEFAULT_ALL to btk_drag_dest_set()
   * when using the drag-motion signal that way.
   *
   * Also note that there is no drag-enter signal. The drag receiver has to
   * keep track of whether he has received any drag-motion signals since the
   * last #BtkWidget::drag-leave and if not, treat the drag-motion signal as
   * an "enter" signal. Upon an "enter", the handler will typically highlight
   * the drop site with btk_drag_highlight().
   * |[
   * static void
   * drag_motion (BtkWidget *widget,
   *              BdkDragContext *context,
   *              gint x,
   *              gint y,
   *              guint time)
   * {
   *   BdkAtom target;
   *  
   *   PrivateData *private_data = GET_PRIVATE_DATA (widget);
   *  
   *   if (!private_data->drag_highlight) 
   *    {
   *      private_data->drag_highlight = 1;
   *      btk_drag_highlight (widget);
   *    }
   *  
   *   target = btk_drag_dest_find_target (widget, context, NULL);
   *   if (target == BDK_NONE)
   *     bdk_drag_status (context, 0, time);
   *   else 
   *    {
   *      private_data->pending_status = context->suggested_action;
   *      btk_drag_get_data (widget, context, target, time);
   *    }
   *  
   *   return TRUE;
   * }
   *   
   * static void
   * drag_data_received (BtkWidget        *widget,
   *                     BdkDragContext   *context,
   *                     gint              x,
   *                     gint              y,
   *                     BtkSelectionData *selection_data,
   *                     guint             info,
   *                     guint             time)
   * {
   *   PrivateData *private_data = GET_PRIVATE_DATA (widget);
   *   
   *   if (private_data->suggested_action) 
   *    {
   *      private_data->suggested_action = 0;
   *      
   *     /&ast; We are getting this data due to a request in drag_motion,
   *      * rather than due to a request in drag_drop, so we are just
   *      * supposed to call bdk_drag_status (), not actually paste in 
   *      * the data.
   *      &ast;/
   *      str = btk_selection_data_get_text (selection_data);
   *      if (!data_is_acceptable (str)) 
   *        bdk_drag_status (context, 0, time);
   *      else
   *        bdk_drag_status (context, private_data->suggested_action, time);
   *    }
   *   else
   *    {
   *      /&ast; accept the drop &ast;/
   *    }
   * }
   * ]|
   */
  widget_signals[DRAG_MOTION] =
    g_signal_new (I_("drag-motion"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_motion),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__OBJECT_INT_INT_UINT,
		  G_TYPE_BOOLEAN, 4,
		  BDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_UINT);

  /**
   * BtkWidget::drag-drop:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   * @x: the x coordinate of the current cursor position
   * @y: the y coordinate of the current cursor position
   * @time: the timestamp of the motion event
   * @returns: whether the cursor position is in a drop zone
   *
   * The ::drag-drop signal is emitted on the drop site when the user drops 
   * the data onto the widget. The signal handler must determine whether 
   * the cursor position is in a drop zone or not. If it is not in a drop 
   * zone, it returns %FALSE and no further processing is necessary. 
   * Otherwise, the handler returns %TRUE. In this case, the handler must 
   * ensure that btk_drag_finish() is called to let the source know that 
   * the drop is done. The call to btk_drag_finish() can be done either 
   * directly or in a #BtkWidget::drag-data-received handler which gets 
   * triggered by calling btk_drag_get_data() to receive the data for one 
   * or more of the supported targets.
   */
  widget_signals[DRAG_DROP] =
    g_signal_new (I_("drag-drop"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_drop),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__OBJECT_INT_INT_UINT,
		  G_TYPE_BOOLEAN, 4,
		  BDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_UINT);

  /**
   * BtkWidget::drag-data-get:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   * @data: the #BtkSelectionData to be filled with the dragged data
   * @info: the info that has been registered with the target in the 
   *        #BtkTargetList
   * @time: the timestamp at which the data was requested
   *
   * The ::drag-data-get signal is emitted on the drag source when the drop 
   * site requests the data which is dragged. It is the responsibility of 
   * the signal handler to fill @data with the data in the format which 
   * is indicated by @info. See btk_selection_data_set() and 
   * btk_selection_data_set_text().
   */
  widget_signals[DRAG_DATA_GET] =
    g_signal_new (I_("drag-data-get"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_data_get),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_BOXED_UINT_UINT,
		  G_TYPE_NONE, 4,
		  BDK_TYPE_DRAG_CONTEXT,
		  BTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT,
		  G_TYPE_UINT);

  /**
   * BtkWidget::drag-data-received:
   * @widget: the object which received the signal
   * @drag_context: the drag context
   * @x: where the drop happened
   * @y: where the drop happened
   * @data: the received data
   * @info: the info that has been registered with the target in the 
   *        #BtkTargetList
   * @time: the timestamp at which the data was received
   *
   * The ::drag-data-received signal is emitted on the drop site when the 
   * dragged data has been received. If the data was received in order to 
   * determine whether the drop will be accepted, the handler is expected 
   * to call bdk_drag_status() and <emphasis>not</emphasis> finish the drag. 
   * If the data was received in response to a #BtkWidget::drag-drop signal 
   * (and this is the last target to be received), the handler for this 
   * signal is expected to process the received data and then call 
   * btk_drag_finish(), setting the @success parameter depending on whether 
   * the data was processed successfully. 
   * 
   * The handler may inspect and modify @drag_context->action before calling 
   * btk_drag_finish(), e.g. to implement %BDK_ACTION_ASK as shown in the 
   * following example:
   * |[
   * void  
   * drag_data_received (BtkWidget          *widget,
   *                     BdkDragContext     *drag_context,
   *                     gint                x,
   *                     gint                y,
   *                     BtkSelectionData   *data,
   *                     guint               info,
   *                     guint               time)
   * {
   *   if ((data->length >= 0) && (data->format == 8))
   *     {
   *       if (drag_context->action == BDK_ACTION_ASK) 
   *         {
   *           BtkWidget *dialog;
   *           gint response;
   *           
   *           dialog = btk_message_dialog_new (NULL,
   *                                            BTK_DIALOG_MODAL | 
   *                                            BTK_DIALOG_DESTROY_WITH_PARENT,
   *                                            BTK_MESSAGE_INFO,
   *                                            BTK_BUTTONS_YES_NO,
   *                                            "Move the data ?\n");
   *           response = btk_dialog_run (BTK_DIALOG (dialog));
   *           btk_widget_destroy (dialog);
   *             
   *           if (response == BTK_RESPONSE_YES)
   *             drag_context->action = BDK_ACTION_MOVE;
   *           else
   *             drag_context->action = BDK_ACTION_COPY;
   *          }
   *          
   *       btk_drag_finish (drag_context, TRUE, FALSE, time);
   *       return;
   *     }
   *       
   *    btk_drag_finish (drag_context, FALSE, FALSE, time);
   *  }
   * ]|
   */
  widget_signals[DRAG_DATA_RECEIVED] =
    g_signal_new (I_("drag-data-received"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, drag_data_received),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_INT_INT_BOXED_UINT_UINT,
		  G_TYPE_NONE, 6,
		  BDK_TYPE_DRAG_CONTEXT,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  BTK_TYPE_SELECTION_DATA | G_SIGNAL_TYPE_STATIC_SCOPE,
		  G_TYPE_UINT,
		  G_TYPE_UINT);
  
  /**
   * BtkWidget::visibility-notify-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventVisibility): the #BdkEventVisibility which
   *   triggered this signal.
   *
   * The ::visibility-notify-event will be emitted when the @widget's window
   * is obscured or unobscured.
   *
   * To receive this signal the #BdkWindow associated to the widget needs
   * to enable the #BDK_VISIBILITY_NOTIFY_MASK mask.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[VISIBILITY_NOTIFY_EVENT] =
    g_signal_new (I_("visibility-notify-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, visibility_notify_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::client-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventClient): the #BdkEventClient which triggered
   *   this signal.
   *
   * The ::client-event will be emitted when the @widget's window
   * receives a message (via a ClientMessage event) from another
   * application.
   *
   * Returns: %TRUE to stop other handlers from being invoked for 
   *   the event. %FALSE to propagate the event further.
   */
  widget_signals[CLIENT_EVENT] =
    g_signal_new (I_("client-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, client_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::no-expose-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventNoExpose): the #BdkEventNoExpose which triggered
   *   this signal.
   *
   * The ::no-expose-event will be emitted when the @widget's window is 
   * drawn as a copy of another #BdkDrawable (with bdk_draw_drawable() or
   * bdk_window_copy_area()) which was completely unobscured. If the source
   * window was partially obscured #BdkEventExpose events will be generated
   * for those areas.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. 
   *   %FALSE to propagate the event further.
   */
  widget_signals[NO_EXPOSE_EVENT] =
    g_signal_new (I_("no-expose-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, no_expose_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::window-state-event:
   * @widget: the object which received the signal
   * @event: (type Bdk.EventWindowState): the #BdkEventWindowState which
   *   triggered this signal.
   *
   * The ::window-state-event will be emitted when the state of the 
   * toplevel window associated to the @widget changes.
   *
   * To receive this signal the #BdkWindow associated to the widget 
   * needs to enable the #BDK_STRUCTURE_MASK mask. BDK will enable 
   * this mask automatically for all new windows.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the 
   *   event. %FALSE to propagate the event further.
   */
  widget_signals[WINDOW_STATE_EVENT] =
    g_signal_new (I_("window-state-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, window_state_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  /**
   * BtkWidget::damage-event:
   * @widget: the object which received the signal
   * @event: the #BdkEventExpose event
   *
   * Emitted when a redirected window belonging to @widget gets drawn into.
   * The rebunnyion/area members of the event shows what area of the redirected
   * drawable was drawn into.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   *
   * Since: 2.14
   */
  widget_signals[DAMAGE_EVENT] =
    g_signal_new (I_("damage-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
                  0,
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
/**
   * BtkWidget::grab-broken-event:
   * @widget: the object which received the signal
   * @event: the #BdkEventGrabBroken event
   *
   * Emitted when a pointer or keyboard grab on a window belonging 
   * to @widget gets broken. 
   * 
   * On X11, this happens when the grab window becomes unviewable 
   * (i.e. it or one of its ancestors is unmapped), or if the same 
   * application grabs the pointer or keyboard again.
   *
   * Returns: %TRUE to stop other handlers from being invoked for 
   *   the event. %FALSE to propagate the event further.
   *
   * Since: 2.8
   */
  widget_signals[GRAB_BROKEN] =
    g_signal_new (I_("grab-broken-event"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, grab_broken_event),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__BOXED,
		  G_TYPE_BOOLEAN, 1,
		  BDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
  /**
   * BtkWidget::query-tooltip:
   * @widget: the object which received the signal
   * @x: the x coordinate of the cursor position where the request has 
   *     been emitted, relative to @widget->window
   * @y: the y coordinate of the cursor position where the request has 
   *     been emitted, relative to @widget->window
   * @keyboard_mode: %TRUE if the tooltip was trigged using the keyboard
   * @tooltip: a #BtkTooltip
   *
   * Emitted when #BtkWidget:has-tooltip is %TRUE and the #BtkSettings:btk-tooltip-timeout 
   * has expired with the cursor hovering "above" @widget; or emitted when @widget got 
   * focus in keyboard mode.
   *
   * Using the given coordinates, the signal handler should determine
   * whether a tooltip should be shown for @widget. If this is the case
   * %TRUE should be returned, %FALSE otherwise.  Note that if
   * @keyboard_mode is %TRUE, the values of @x and @y are undefined and
   * should not be used.
   *
   * The signal handler is free to manipulate @tooltip with the therefore
   * destined function calls.
   *
   * Returns: %TRUE if @tooltip should be shown right now, %FALSE otherwise.
   *
   * Since: 2.12
   */
  widget_signals[QUERY_TOOLTIP] =
    g_signal_new (I_("query-tooltip"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, query_tooltip),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__INT_INT_BOOLEAN_OBJECT,
		  G_TYPE_BOOLEAN, 4,
		  G_TYPE_INT,
		  G_TYPE_INT,
		  G_TYPE_BOOLEAN,
		  BTK_TYPE_TOOLTIP);

  /**
   * BtkWidget::popup-menu
   * @widget: the object which received the signal
   *
   * This signal gets emitted whenever a widget should pop up a context 
   * menu. This usually happens through the standard key binding mechanism; 
   * by pressing a certain key while a widget is focused, the user can cause 
   * the widget to pop up a menu.  For example, the #BtkEntry widget creates 
   * a menu with clipboard commands. See <xref linkend="checklist-popup-menu"/> 
   * for an example of how to use this signal.
   *
   * Returns: %TRUE if a menu was activated
   */
  widget_signals[POPUP_MENU] =
    g_signal_new (I_("popup-menu"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkWidgetClass, popup_menu),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__VOID,
		  G_TYPE_BOOLEAN, 0);

  /**
   * BtkWidget::show-help:
   * @widget: the object which received the signal.
   * @help_type:
   */
  widget_signals[SHOW_HELP] =
    g_signal_new (I_("show-help"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkWidgetClass, show_help),
		  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__ENUM,
		  G_TYPE_BOOLEAN, 1,
		  BTK_TYPE_WIDGET_HELP_TYPE);

  /**
   * BtkWidget::accel-closures-changed:
   * @widget: the object which received the signal.
   */
  widget_signals[ACCEL_CLOSURES_CHANGED] =
    g_signal_new (I_("accel-closures-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  0,
		  0,
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  /**
   * BtkWidget::screen-changed:
   * @widget: the object on which the signal is emitted
   * @previous_screen: (allow-none): the previous screen, or %NULL if the
   *   widget was not associated with a screen before
   *
   * The ::screen-changed signal gets emitted when the
   * screen of a widget has changed.
   */
  widget_signals[SCREEN_CHANGED] =
    g_signal_new (I_("screen-changed"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, screen_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT,
		  G_TYPE_NONE, 1,
		  BDK_TYPE_SCREEN);

  /**
   * BtkWidget::can-activate-accel:
   * @widget: the object which received the signal
   * @signal_id: the ID of a signal installed on @widget
   *
   * Determines whether an accelerator that activates the signal
   * identified by @signal_id can currently be activated.
   * This signal is present to allow applications and derived
   * widgets to override the default #BtkWidget handling
   * for determining whether an accelerator can be activated.
   *
   * Returns: %TRUE if the signal can be activated.
   */
  widget_signals[CAN_ACTIVATE_ACCEL] =
     g_signal_new (I_("can-activate-accel"),
		  G_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkWidgetClass, can_activate_accel),
                  _btk_boolean_handled_accumulator, NULL,
		  _btk_marshal_BOOLEAN__UINT,
                  G_TYPE_BOOLEAN, 1, G_TYPE_UINT);

  binding_set = btk_binding_set_by_class (klass);
  btk_binding_entry_add_signal (binding_set, BDK_F10, BDK_SHIFT_MASK,
                                "popup-menu", 0);
  btk_binding_entry_add_signal (binding_set, BDK_Menu, 0,
                                "popup-menu", 0);  

  btk_binding_entry_add_signal (binding_set, BDK_F1, BDK_CONTROL_MASK,
                                "show-help", 1,
                                BTK_TYPE_WIDGET_HELP_TYPE,
                                BTK_WIDGET_HELP_TOOLTIP);
  btk_binding_entry_add_signal (binding_set, BDK_KP_F1, BDK_CONTROL_MASK,
                                "show-help", 1,
                                BTK_TYPE_WIDGET_HELP_TYPE,
                                BTK_WIDGET_HELP_TOOLTIP);
  btk_binding_entry_add_signal (binding_set, BDK_F1, BDK_SHIFT_MASK,
                                "show-help", 1,
                                BTK_TYPE_WIDGET_HELP_TYPE,
                                BTK_WIDGET_HELP_WHATS_THIS);  
  btk_binding_entry_add_signal (binding_set, BDK_KP_F1, BDK_SHIFT_MASK,
                                "show-help", 1,
                                BTK_TYPE_WIDGET_HELP_TYPE,
                                BTK_WIDGET_HELP_WHATS_THIS);

  btk_widget_class_install_style_property (klass,
					   g_param_spec_boolean ("interior-focus",
								 P_("Interior Focus"),
								 P_("Whether to draw the focus indicator inside widgets"),
								 TRUE,
								 BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (klass,
					   g_param_spec_int ("focus-line-width",
							     P_("Focus linewidth"),
							     P_("Width, in pixels, of the focus indicator line"),
							     0, G_MAXINT, 1,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (klass,
					   g_param_spec_string ("focus-line-pattern",
								P_("Focus line dash pattern"),
								P_("Dash pattern used to draw the focus indicator"),
								"\1\1",
								BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (klass,
					   g_param_spec_int ("focus-padding",
							     P_("Focus padding"),
							     P_("Width, in pixels, between focus indicator and the widget 'box'"),
							     0, G_MAXINT, 1,
							     BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("cursor-color",
							       P_("Cursor color"),
							       P_("Color with which to draw insertion cursor"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("secondary-cursor-color",
							       P_("Secondary cursor color"),
							       P_("Color with which to draw the secondary insertion cursor when editing mixed right-to-left and left-to-right text"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (klass,
					   g_param_spec_float ("cursor-aspect-ratio",
							       P_("Cursor line aspect ratio"),
							       P_("Aspect ratio with which to draw insertion cursor"),
							       0.0, 1.0, 0.04,
							       BTK_PARAM_READABLE));

  /**
   * BtkWidget:draw-border:
   *
   * The "draw-border" style property defines the size of areas outside 
   * the widget's allocation to draw.
   *
   * Since: 2.8
   *
   * Deprecated: 2.22: This property will be removed in BTK+ 3
   */
  btk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("draw-border",
							       P_("Draw Border"),
							       P_("Size of areas outside the widget's allocation to draw"),
							       BTK_TYPE_BORDER,
							       BTK_PARAM_READABLE));

  /**
   * BtkWidget:link-color:
   *
   * The "link-color" style property defines the color of unvisited links.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("link-color",
							       P_("Unvisited Link Color"),
							       P_("Color of unvisited links"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));

  /**
   * BtkWidget:visited-link-color:
   *
   * The "visited-link-color" style property defines the color of visited links.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
					   g_param_spec_boxed ("visited-link-color",
							       P_("Visited Link Color"),
							       P_("Color of visited links"),
							       BDK_TYPE_COLOR,
							       BTK_PARAM_READABLE));

  /**
   * BtkWidget:wide-separators:
   *
   * The "wide-separators" style property defines whether separators have 
   * configurable width and should be drawn using a box instead of a line.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_boolean ("wide-separators",
                                                                 P_("Wide Separators"),
                                                                 P_("Whether separators have configurable width and should be drawn using a box instead of a line"),
                                                                 FALSE,
                                                                 BTK_PARAM_READABLE));

  /**
   * BtkWidget:separator-width:
   *
   * The "separator-width" style property defines the width of separators.
   * This property only takes effect if #BtkWidget:wide-separators is %TRUE.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("separator-width",
                                                             P_("Separator Width"),
                                                             P_("The width of separators if wide-separators is TRUE"),
                                                             0, G_MAXINT, 0,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkWidget:separator-height:
   *
   * The "separator-height" style property defines the height of separators.
   * This property only takes effect if #BtkWidget:wide-separators is %TRUE.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("separator-height",
                                                             P_("Separator Height"),
                                                             P_("The height of separators if \"wide-separators\" is TRUE"),
                                                             0, G_MAXINT, 0,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkWidget:scroll-arrow-hlength:
   *
   * The "scroll-arrow-hlength" style property defines the length of 
   * horizontal scroll arrows.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("scroll-arrow-hlength",
                                                             P_("Horizontal Scroll Arrow Length"),
                                                             P_("The length of horizontal scroll arrows"),
                                                             1, G_MAXINT, 16,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkWidget:scroll-arrow-vlength:
   *
   * The "scroll-arrow-vlength" style property defines the length of 
   * vertical scroll arrows.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_int ("scroll-arrow-vlength",
                                                             P_("Vertical Scroll Arrow Length"),
                                                             P_("The length of vertical scroll arrows"),
                                                             1, G_MAXINT, 16,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkWidget:tooltip-alpha:
   *
   * The "tooltip-alpha" style property defines the opacity of
   * widget tooltips.
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_uchar ("tooltip-alpha",
                                                               P_("Tooltips opacity"),
                                                               P_("The opacity to be used when drawing tooltips"),
                                                               0, 255, 255,
                                                               BTK_PARAM_READABLE));

  /**
   * BtkWidget:tooltip-radius:
   *
   * The "tooltip-radius" style property defines the radius of
   * widget tooltips.
   */
  btk_widget_class_install_style_property (klass,
                                           g_param_spec_uint ("tooltip-radius",
                                                              P_("Tooltips radius"),
                                                              P_("The radius to be used when drawing tooltips"),
                                                              0, G_MAXINT, 0,
                                                              BTK_PARAM_READABLE));
}

static void
btk_widget_base_class_finalize (BtkWidgetClass *klass)
{
  GList *list, *node;

  list = g_param_spec_pool_list_owned (style_property_spec_pool, G_OBJECT_CLASS_TYPE (klass));
  for (node = list; node; node = node->next)
    {
      GParamSpec *pspec = node->data;

      g_param_spec_pool_remove (style_property_spec_pool, pspec);
      g_param_spec_unref (pspec);
    }
  g_list_free (list);
}

static void
btk_widget_set_property (GObject         *object,
			 guint            prop_id,
			 const GValue    *value,
			 GParamSpec      *pspec)
{
  BtkWidget *widget = BTK_WIDGET (object);

  switch (prop_id)
    {
      gboolean tmp;
      gchar *tooltip_markup;
      const gchar *tooltip_text;
      BtkWindow *tooltip_window;

    case PROP_NAME:
      btk_widget_set_name (widget, g_value_get_string (value));
      break;
    case PROP_PARENT:
      btk_container_add (BTK_CONTAINER (g_value_get_object (value)), widget);
      break;
    case PROP_WIDTH_REQUEST:
      btk_widget_set_usize_internal (widget, g_value_get_int (value), -2);
      break;
    case PROP_HEIGHT_REQUEST:
      btk_widget_set_usize_internal (widget, -2, g_value_get_int (value));
      break;
    case PROP_VISIBLE:
      btk_widget_set_visible (widget, g_value_get_boolean (value));
      break;
    case PROP_SENSITIVE:
      btk_widget_set_sensitive (widget, g_value_get_boolean (value));
      break;
    case PROP_APP_PAINTABLE:
      btk_widget_set_app_paintable (widget, g_value_get_boolean (value));
      break;
    case PROP_CAN_FOCUS:
      btk_widget_set_can_focus (widget, g_value_get_boolean (value));
      break;
    case PROP_HAS_FOCUS:
      if (g_value_get_boolean (value))
	btk_widget_grab_focus (widget);
      break;
    case PROP_IS_FOCUS:
      if (g_value_get_boolean (value))
	btk_widget_grab_focus (widget);
      break;
    case PROP_CAN_DEFAULT:
      btk_widget_set_can_default (widget, g_value_get_boolean (value));
      break;
    case PROP_HAS_DEFAULT:
      if (g_value_get_boolean (value))
	btk_widget_grab_default (widget);
      break;
    case PROP_RECEIVES_DEFAULT:
      btk_widget_set_receives_default (widget, g_value_get_boolean (value));
      break;
    case PROP_STYLE:
      btk_widget_set_style (widget, g_value_get_object (value));
      break;
    case PROP_EVENTS:
      if (!btk_widget_get_realized (widget) && btk_widget_get_has_window (widget))
	btk_widget_set_events (widget, g_value_get_flags (value));
      break;
    case PROP_EXTENSION_EVENTS:
      btk_widget_set_extension_events (widget, g_value_get_enum (value));
      break;
    case PROP_NO_SHOW_ALL:
      btk_widget_set_no_show_all (widget, g_value_get_boolean (value));
      break;
    case PROP_HAS_TOOLTIP:
      btk_widget_real_set_has_tooltip (widget,
				       g_value_get_boolean (value), FALSE);
      break;
    case PROP_TOOLTIP_MARKUP:
      tooltip_window = g_object_get_qdata (object, quark_tooltip_window);
      tooltip_markup = g_value_dup_string (value);

      /* Treat an empty string as a NULL string, 
       * because an empty string would be useless for a tooltip:
       */
      if (tooltip_markup && (strlen (tooltip_markup) == 0))
        {
	  g_free (tooltip_markup);
          tooltip_markup = NULL;
        }

      g_object_set_qdata_full (object, quark_tooltip_markup,
			       tooltip_markup, g_free);

      tmp = (tooltip_window != NULL || tooltip_markup != NULL);
      btk_widget_real_set_has_tooltip (widget, tmp, FALSE);
      if (btk_widget_get_visible (widget))
        btk_widget_queue_tooltip_query (widget);
      break;
    case PROP_TOOLTIP_TEXT:
      tooltip_window = g_object_get_qdata (object, quark_tooltip_window);

      tooltip_text = g_value_get_string (value);

      /* Treat an empty string as a NULL string, 
       * because an empty string would be useless for a tooltip:
       */
      if (tooltip_text && (strlen (tooltip_text) == 0))
        tooltip_text = NULL;

      tooltip_markup = tooltip_text ? g_markup_escape_text (tooltip_text, -1) : NULL;

      g_object_set_qdata_full (object, quark_tooltip_markup,
                               tooltip_markup, g_free);

      tmp = (tooltip_window != NULL || tooltip_markup != NULL);
      btk_widget_real_set_has_tooltip (widget, tmp, FALSE);
      if (btk_widget_get_visible (widget))
        btk_widget_queue_tooltip_query (widget);
      break;
    case PROP_DOUBLE_BUFFERED:
      btk_widget_set_double_buffered (widget, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_widget_get_property (GObject         *object,
			 guint            prop_id,
			 GValue          *value,
			 GParamSpec      *pspec)
{
  BtkWidget *widget = BTK_WIDGET (object);
  
  switch (prop_id)
    {
      gpointer *eventp;
      gpointer *modep;

    case PROP_NAME:
      if (widget->name)
	g_value_set_string (value, widget->name);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_PARENT:
      g_value_set_object (value, widget->parent);
      break;
    case PROP_WIDTH_REQUEST:
      {
        int w;
        btk_widget_get_size_request (widget, &w, NULL);
        g_value_set_int (value, w);
      }
      break;
    case PROP_HEIGHT_REQUEST:
      {
        int h;
        btk_widget_get_size_request (widget, NULL, &h);
        g_value_set_int (value, h);
      }
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, (btk_widget_get_visible (widget) != FALSE));
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, (btk_widget_get_sensitive (widget) != FALSE));
      break;
    case PROP_APP_PAINTABLE:
      g_value_set_boolean (value, (btk_widget_get_app_paintable (widget) != FALSE));
      break;
    case PROP_CAN_FOCUS:
      g_value_set_boolean (value, (btk_widget_get_can_focus (widget) != FALSE));
      break;
    case PROP_HAS_FOCUS:
      g_value_set_boolean (value, (btk_widget_has_focus (widget) != FALSE));
      break;
    case PROP_IS_FOCUS:
      g_value_set_boolean (value, (btk_widget_is_focus (widget)));
      break;
    case PROP_CAN_DEFAULT:
      g_value_set_boolean (value, (btk_widget_get_can_default (widget) != FALSE));
      break;
    case PROP_HAS_DEFAULT:
      g_value_set_boolean (value, (btk_widget_has_default (widget) != FALSE));
      break;
    case PROP_RECEIVES_DEFAULT:
      g_value_set_boolean (value, (btk_widget_get_receives_default (widget) != FALSE));
      break;
    case PROP_COMPOSITE_CHILD:
      g_value_set_boolean (value, (BTK_OBJECT_FLAGS (widget) & BTK_COMPOSITE_CHILD) != 0 );
      break;
    case PROP_STYLE:
      g_value_set_object (value, btk_widget_get_style (widget));
      break;
    case PROP_EVENTS:
      eventp = g_object_get_qdata (G_OBJECT (widget), quark_event_mask);
      g_value_set_flags (value, GPOINTER_TO_INT (eventp));
      break;
    case PROP_EXTENSION_EVENTS:
      modep = g_object_get_qdata (G_OBJECT (widget), quark_extension_event_mode);
      g_value_set_enum (value, GPOINTER_TO_INT (modep));
      break;
    case PROP_NO_SHOW_ALL:
      g_value_set_boolean (value, btk_widget_get_no_show_all (widget));
      break;
    case PROP_HAS_TOOLTIP:
      g_value_set_boolean (value, GPOINTER_TO_UINT (g_object_get_qdata (object, quark_has_tooltip)));
      break;
    case PROP_TOOLTIP_TEXT:
      {
        gchar *escaped = g_object_get_qdata (object, quark_tooltip_markup);
        gchar *text = NULL;

        if (escaped && !bango_parse_markup (escaped, -1, 0, NULL, &text, NULL, NULL))
          g_assert (NULL == text); /* text should still be NULL in case of markup errors */

        g_value_take_string (value, text);
      }
      break;
    case PROP_TOOLTIP_MARKUP:
      g_value_set_string (value, g_object_get_qdata (object, quark_tooltip_markup));
      break;
    case PROP_WINDOW:
      g_value_set_object (value, btk_widget_get_window (widget));
      break;
    case PROP_DOUBLE_BUFFERED:
      g_value_set_boolean (value, btk_widget_get_double_buffered (widget));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_widget_init (BtkWidget *widget)
{
  BTK_PRIVATE_FLAGS (widget) = PRIVATE_BTK_CHILD_VISIBLE;
  widget->state = BTK_STATE_NORMAL;
  widget->saved_state = BTK_STATE_NORMAL;
  widget->name = NULL;
  widget->requisition.width = 0;
  widget->requisition.height = 0;
  widget->allocation.x = -1;
  widget->allocation.y = -1;
  widget->allocation.width = 1;
  widget->allocation.height = 1;
  widget->window = NULL;
  widget->parent = NULL;

  BTK_OBJECT_FLAGS (widget) |= BTK_SENSITIVE;
  BTK_OBJECT_FLAGS (widget) |= BTK_PARENT_SENSITIVE;
  BTK_OBJECT_FLAGS (widget) |= composite_child_stack ? BTK_COMPOSITE_CHILD : 0;
  btk_widget_set_double_buffered (widget, TRUE);

  BTK_PRIVATE_SET_FLAG (widget, BTK_REDRAW_ON_ALLOC);
  BTK_PRIVATE_SET_FLAG (widget, BTK_REQUEST_NEEDED);
  BTK_PRIVATE_SET_FLAG (widget, BTK_ALLOC_NEEDED);

  widget->style = btk_widget_get_default_style ();
  g_object_ref (widget->style);
}


static void
btk_widget_dispatch_child_properties_changed (BtkWidget   *widget,
					      guint        n_pspecs,
					      GParamSpec **pspecs)
{
  BtkWidget *container = widget->parent;
  guint i;

  for (i = 0; widget->parent == container && i < n_pspecs; i++)
    g_signal_emit (widget, widget_signals[CHILD_NOTIFY], g_quark_from_string (pspecs[i]->name), pspecs[i]);
}

/**
 * btk_widget_freeze_child_notify:
 * @widget: a #BtkWidget
 * 
 * Stops emission of #BtkWidget::child-notify signals on @widget. The 
 * signals are queued until btk_widget_thaw_child_notify() is called 
 * on @widget. 
 *
 * This is the analogue of g_object_freeze_notify() for child properties.
 **/
void
btk_widget_freeze_child_notify (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (!G_OBJECT (widget)->ref_count)
    return;

  g_object_ref (widget);
  g_object_notify_queue_freeze (G_OBJECT (widget), _btk_widget_child_property_notify_context);
  g_object_unref (widget);
}

/**
 * btk_widget_child_notify:
 * @widget: a #BtkWidget
 * @child_property: the name of a child property installed on the 
 *                  class of @widget<!-- -->'s parent
 * 
 * Emits a #BtkWidget::child-notify signal for the 
 * <link linkend="child-properties">child property</link> @child_property 
 * on @widget.
 *
 * This is the analogue of g_object_notify() for child properties.
 **/
void
btk_widget_child_notify (BtkWidget    *widget,
			 const gchar  *child_property)
{
  GParamSpec *pspec;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (child_property != NULL);
  if (!G_OBJECT (widget)->ref_count || !widget->parent)
    return;

  g_object_ref (widget);
  pspec = g_param_spec_pool_lookup (_btk_widget_child_property_pool,
				    child_property,
				    G_OBJECT_TYPE (widget->parent),
				    TRUE);
  if (!pspec)
    g_warning ("%s: container class `%s' has no child property named `%s'",
	       B_STRLOC,
	       G_OBJECT_TYPE_NAME (widget->parent),
	       child_property);
  else
    {
      GObjectNotifyQueue *nqueue = g_object_notify_queue_freeze (G_OBJECT (widget), _btk_widget_child_property_notify_context);

      g_object_notify_queue_add (G_OBJECT (widget), nqueue, pspec);
      g_object_notify_queue_thaw (G_OBJECT (widget), nqueue);
    }
  g_object_unref (widget);
}

/**
 * btk_widget_thaw_child_notify:
 * @widget: a #BtkWidget
 *
 * Reverts the effect of a previous call to btk_widget_freeze_child_notify().
 * This causes all queued #BtkWidget::child-notify signals on @widget to be 
 * emitted.
 */ 
void
btk_widget_thaw_child_notify (BtkWidget *widget)
{
  GObjectNotifyQueue *nqueue;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (!G_OBJECT (widget)->ref_count)
    return;

  g_object_ref (widget);
  nqueue = g_object_notify_queue_from_object (G_OBJECT (widget), _btk_widget_child_property_notify_context);
  if (!nqueue || !nqueue->freeze_count)
    g_warning (B_STRLOC ": child-property-changed notification for %s(%p) is not frozen",
	       G_OBJECT_TYPE_NAME (widget), widget);
  else
    g_object_notify_queue_thaw (G_OBJECT (widget), nqueue);
  g_object_unref (widget);
}


/**
 * btk_widget_new:
 * @type: type ID of the widget to create
 * @first_property_name: name of first property to set
 * @Varargs: value of first property, followed by more properties, 
 *           %NULL-terminated
 * 
 * This is a convenience function for creating a widget and setting
 * its properties in one go. For example you might write:
 * <literal>btk_widget_new (BTK_TYPE_LABEL, "label", "Hello World", "xalign",
 * 0.0, NULL)</literal> to create a left-aligned label. Equivalent to
 * g_object_new(), but returns a widget so you don't have to
 * cast the object yourself.
 * 
 * Return value: a new #BtkWidget of type @widget_type
 **/
BtkWidget*
btk_widget_new (GType        type,
		const gchar *first_property_name,
		...)
{
  BtkWidget *widget;
  va_list var_args;
  
  g_return_val_if_fail (g_type_is_a (type, BTK_TYPE_WIDGET), NULL);
  
  va_start (var_args, first_property_name);
  widget = (BtkWidget *)g_object_new_valist (type, first_property_name, var_args);
  va_end (var_args);

  return widget;
}

/**
 * btk_widget_set:
 * @widget: a #BtkWidget
 * @first_property_name: name of first property to set
 * @Varargs: value of first property, followed by more properties, 
 *           %NULL-terminated
 * 
 * Precursor of g_object_set().
 *
 * Deprecated: 2.0: Use g_object_set() instead.
 **/
void
btk_widget_set (BtkWidget   *widget,
		const gchar *first_property_name,
		...)
{
  va_list var_args;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  va_start (var_args, first_property_name);
  g_object_set_valist (G_OBJECT (widget), first_property_name, var_args);
  va_end (var_args);
}

static inline void	   
btk_widget_queue_draw_child (BtkWidget *widget)
{
  BtkWidget *parent;

  parent = widget->parent;
  if (parent && btk_widget_is_drawable (parent))
    btk_widget_queue_draw_area (parent,
				widget->allocation.x,
				widget->allocation.y,
				widget->allocation.width,
				widget->allocation.height);
}

/**
 * btk_widget_unparent:
 * @widget: a #BtkWidget
 * 
 * This function is only for use in widget implementations.
 * Should be called by implementations of the remove method
 * on #BtkContainer, to dissociate a child from the container.
 **/
void
btk_widget_unparent (BtkWidget *widget)
{
  GObjectNotifyQueue *nqueue;
  BtkWidget *toplevel;
  BtkWidget *old_parent;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  if (widget->parent == NULL)
    return;
  
  /* keep this function in sync with btk_menu_detach()
   */

  g_object_freeze_notify (G_OBJECT (widget));
  nqueue = g_object_notify_queue_freeze (G_OBJECT (widget), _btk_widget_child_property_notify_context);

  toplevel = btk_widget_get_toplevel (widget);
  if (btk_widget_is_toplevel (toplevel))
    _btk_window_unset_focus_and_default (BTK_WINDOW (toplevel), widget);

  if (BTK_CONTAINER (widget->parent)->focus_child == widget)
    btk_container_set_focus_child (BTK_CONTAINER (widget->parent), NULL);

  /* If we are unanchoring the child, we save around the toplevel
   * to emit hierarchy changed
   */
  if (BTK_WIDGET_ANCHORED (widget->parent))
    g_object_ref (toplevel);
  else
    toplevel = NULL;

  btk_widget_queue_draw_child (widget);

  /* Reset the width and height here, to force reallocation if we
   * get added back to a new parent. This won't work if our new
   * allocation is smaller than 1x1 and we actually want a size of 1x1...
   * (would 0x0 be OK here?)
   */
  widget->allocation.width = 1;
  widget->allocation.height = 1;
  
  if (btk_widget_get_realized (widget))
    {
      if (BTK_WIDGET_IN_REPARENT (widget))
	btk_widget_unmap (widget);
      else
	btk_widget_unrealize (widget);
    }

  /* Removing a widget from a container restores the child visible
   * flag to the default state, so it doesn't affect the child
   * in the next parent.
   */
  BTK_PRIVATE_SET_FLAG (widget, BTK_CHILD_VISIBLE);
    
  old_parent = widget->parent;
  widget->parent = NULL;
  btk_widget_set_parent_window (widget, NULL);
  g_signal_emit (widget, widget_signals[PARENT_SET], 0, old_parent);
  if (toplevel)
    {
      _btk_widget_propagate_hierarchy_changed (widget, toplevel);
      g_object_unref (toplevel);
    }
      
  g_object_notify (G_OBJECT (widget), "parent");
  g_object_thaw_notify (G_OBJECT (widget));
  if (!widget->parent)
    g_object_notify_queue_clear (G_OBJECT (widget), nqueue);
  g_object_notify_queue_thaw (G_OBJECT (widget), nqueue);
  g_object_unref (widget);
}

/**
 * btk_widget_destroy:
 * @widget: a #BtkWidget
 *
 * Destroys a widget. Equivalent to btk_object_destroy(), except that
 * you don't have to cast the widget to #BtkObject. When a widget is
 * destroyed, it will break any references it holds to other objects.
 * If the widget is inside a container, the widget will be removed
 * from the container. If the widget is a toplevel (derived from
 * #BtkWindow), it will be removed from the list of toplevels, and the
 * reference BTK+ holds to it will be removed. Removing a
 * widget from its container or the list of toplevels results in the
 * widget being finalized, unless you've added additional references
 * to the widget with g_object_ref().
 *
 * In most cases, only toplevel widgets (windows) require explicit
 * destruction, because when you destroy a toplevel its children will
 * be destroyed as well.
 **/
void
btk_widget_destroy (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  btk_object_destroy ((BtkObject*) widget);
}

/**
 * btk_widget_destroyed:
 * @widget: a #BtkWidget
 * @widget_pointer: (inout) (transfer none): address of a variable that contains @widget
 *
 * This function sets *@widget_pointer to %NULL if @widget_pointer !=
 * %NULL.  It's intended to be used as a callback connected to the
 * "destroy" signal of a widget. You connect btk_widget_destroyed()
 * as a signal handler, and pass the address of your widget variable
 * as user data. Then when the widget is destroyed, the variable will
 * be set to %NULL. Useful for example to avoid multiple copies
 * of the same dialog.
 **/
void
btk_widget_destroyed (BtkWidget      *widget,
		      BtkWidget      **widget_pointer)
{
  /* Don't make any assumptions about the
   *  value of widget!
   *  Even check widget_pointer.
   */
  if (widget_pointer)
    *widget_pointer = NULL;
}

/**
 * btk_widget_show:
 * @widget: a #BtkWidget
 * 
 * Flags a widget to be displayed. Any widget that isn't shown will
 * not appear on the screen. If you want to show all the widgets in a
 * container, it's easier to call btk_widget_show_all() on the
 * container, instead of individually showing the widgets.
 *
 * Remember that you have to show the containers containing a widget,
 * in addition to the widget itself, before it will appear onscreen.
 *
 * When a toplevel container is shown, it is immediately realized and
 * mapped; other shown widgets are realized and mapped when their
 * toplevel container is realized and mapped.
 **/
void
btk_widget_show (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (!btk_widget_get_visible (widget))
    {
      g_object_ref (widget);
      if (!btk_widget_is_toplevel (widget))
	btk_widget_queue_resize (widget);
      g_signal_emit (widget, widget_signals[SHOW], 0);
      g_object_notify (G_OBJECT (widget), "visible");
      g_object_unref (widget);
    }
}

static void
btk_widget_real_show (BtkWidget *widget)
{
  if (!btk_widget_get_visible (widget))
    {
      BTK_WIDGET_SET_FLAGS (widget, BTK_VISIBLE);

      if (widget->parent &&
	  btk_widget_get_mapped (widget->parent) &&
	  BTK_WIDGET_CHILD_VISIBLE (widget) &&
	  !btk_widget_get_mapped (widget))
	btk_widget_map (widget);
    }
}

static void
btk_widget_show_map_callback (BtkWidget *widget, BdkEvent *event, gint *flag)
{
  *flag = TRUE;
  g_signal_handlers_disconnect_by_func (widget,
					btk_widget_show_map_callback, 
					flag);
}

/**
 * btk_widget_show_now:
 * @widget: a #BtkWidget
 * 
 * Shows a widget. If the widget is an unmapped toplevel widget
 * (i.e. a #BtkWindow that has not yet been shown), enter the main
 * loop and wait for the window to actually be mapped. Be careful;
 * because the main loop is running, anything can happen during
 * this function.
 **/
void
btk_widget_show_now (BtkWidget *widget)
{
  gint flag = FALSE;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));

  /* make sure we will get event */
  if (!btk_widget_get_mapped (widget) &&
      btk_widget_is_toplevel (widget))
    {
      btk_widget_show (widget);

      g_signal_connect (widget, "map-event",
			G_CALLBACK (btk_widget_show_map_callback), 
			&flag);

      while (!flag)
	btk_main_iteration ();
    }
  else
    btk_widget_show (widget);
}

/**
 * btk_widget_hide:
 * @widget: a #BtkWidget
 * 
 * Reverses the effects of btk_widget_show(), causing the widget to be
 * hidden (invisible to the user).
 */
void
btk_widget_hide (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  if (btk_widget_get_visible (widget))
    {
      BtkWidget *toplevel = btk_widget_get_toplevel (widget);
      
      g_object_ref (widget);
      if (toplevel != widget && btk_widget_is_toplevel (toplevel))
	_btk_window_unset_focus_and_default (BTK_WINDOW (toplevel), widget);

      g_signal_emit (widget, widget_signals[HIDE], 0);
      if (!btk_widget_is_toplevel (widget))
	btk_widget_queue_resize (widget);
      g_object_notify (G_OBJECT (widget), "visible");
      g_object_unref (widget);
    }
}

static void
btk_widget_real_hide (BtkWidget *widget)
{
  if (btk_widget_get_visible (widget))
    {
      BTK_WIDGET_UNSET_FLAGS (widget, BTK_VISIBLE);
      
      if (btk_widget_get_mapped (widget))
	btk_widget_unmap (widget);
    }
}

/**
 * btk_widget_hide_on_delete:
 * @widget: a #BtkWidget
 * 
 * Utility function; intended to be connected to the #BtkWidget::delete-event
 * signal on a #BtkWindow. The function calls btk_widget_hide() on its
 * argument, then returns %TRUE. If connected to ::delete-event, the
 * result is that clicking the close button for a window (on the
 * window frame, top right corner usually) will hide but not destroy
 * the window. By default, BTK+ destroys windows when ::delete-event
 * is received.
 * 
 * Return value: %TRUE
 **/
gboolean
btk_widget_hide_on_delete (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  
  btk_widget_hide (widget);
  
  return TRUE;
}

/**
 * btk_widget_show_all:
 * @widget: a #BtkWidget
 * 
 * Recursively shows a widget, and any child widgets (if the widget is
 * a container).
 **/
void
btk_widget_show_all (BtkWidget *widget)
{
  BtkWidgetClass *class;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (btk_widget_get_no_show_all (widget))
    return;

  class = BTK_WIDGET_GET_CLASS (widget);

  if (class->show_all)
    class->show_all (widget);
}

/**
 * btk_widget_hide_all:
 * @widget: a #BtkWidget
 * 
 * Recursively hides a widget and any child widgets.
 *
 * Deprecated: 2.24: Use btk_widget_hide() instead.
 */
void
btk_widget_hide_all (BtkWidget *widget)
{
  BtkWidgetClass *class;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (btk_widget_get_no_show_all (widget))
    return;

  class = BTK_WIDGET_GET_CLASS (widget);

  if (class->hide_all)
    class->hide_all (widget);
}

/**
 * btk_widget_map:
 * @widget: a #BtkWidget
 * 
 * This function is only for use in widget implementations. Causes
 * a widget to be mapped if it isn't already.
 **/
void
btk_widget_map (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (btk_widget_get_visible (widget));
  g_return_if_fail (BTK_WIDGET_CHILD_VISIBLE (widget));
  
  if (!btk_widget_get_mapped (widget))
    {
      if (!btk_widget_get_realized (widget))
	btk_widget_realize (widget);

      g_signal_emit (widget, widget_signals[MAP], 0);

      if (!btk_widget_get_has_window (widget))
	bdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
    }
}

/**
 * btk_widget_unmap:
 * @widget: a #BtkWidget
 *
 * This function is only for use in widget implementations. Causes
 * a widget to be unmapped if it's currently mapped.
 **/
void
btk_widget_unmap (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  if (btk_widget_get_mapped (widget))
    {
      if (!btk_widget_get_has_window (widget))
	bdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
      _btk_tooltip_hide (widget);
      g_signal_emit (widget, widget_signals[UNMAP], 0);
    }
}

static void
btk_widget_set_extension_events_internal (BtkWidget        *widget,
                                          BdkExtensionMode  mode,
                                          GList            *window_list)
{
  GList *free_list = NULL;
  GList *l;

  if (window_list == NULL)
    {
      if (btk_widget_get_has_window (widget))
        window_list = g_list_prepend (NULL, widget->window);
      else
        window_list = bdk_window_get_children (widget->window);

      free_list = window_list;
    }

  for (l = window_list; l != NULL; l = l->next)
    {
      BdkWindow *window = l->data;
      gpointer user_data;

      bdk_window_get_user_data (window, &user_data);
      if (user_data == widget)
        {
          GList *children;

          bdk_input_set_extension_events (window,
                                          bdk_window_get_events (window),
                                          mode);

          children = bdk_window_get_children (window);
          if (children)
            {
              btk_widget_set_extension_events_internal (widget, mode, children);
              g_list_free (children);
            }
        }
    }

  if (free_list)
    g_list_free (free_list);
}

/**
 * btk_widget_realize:
 * @widget: a #BtkWidget
 * 
 * Creates the BDK (windowing system) resources associated with a
 * widget.  For example, @widget->window will be created when a widget
 * is realized.  Normally realization happens implicitly; if you show
 * a widget and all its parent containers, then the widget will be
 * realized and mapped automatically.
 * 
 * Realizing a widget requires all
 * the widget's parent widgets to be realized; calling
 * btk_widget_realize() realizes the widget's parents in addition to
 * @widget itself. If a widget is not yet inside a toplevel window
 * when you realize it, bad things will happen.
 *
 * This function is primarily used in widget implementations, and
 * isn't very useful otherwise. Many times when you think you might
 * need it, a better approach is to connect to a signal that will be
 * called after the widget is realized automatically, such as
 * BtkWidget::expose-event. Or simply g_signal_connect () to the
 * BtkWidget::realize signal.
 **/
void
btk_widget_realize (BtkWidget *widget)
{
  BdkExtensionMode mode;
  BtkWidgetShapeInfo *shape_info;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_WIDGET_ANCHORED (widget) ||
		    BTK_IS_INVISIBLE (widget));
  
  if (!btk_widget_get_realized (widget))
    {
      /*
	if (BTK_IS_CONTAINER (widget) && btk_widget_get_has_window (widget))
	  g_message ("btk_widget_realize(%s)", G_OBJECT_TYPE_NAME (widget));
      */

      if (widget->parent == NULL &&
          !btk_widget_is_toplevel (widget))
        g_warning ("Calling btk_widget_realize() on a widget that isn't "
                   "inside a toplevel window is not going to work very well. "
                   "Widgets must be inside a toplevel container before realizing them.");
      
      if (widget->parent && !btk_widget_get_realized (widget->parent))
	btk_widget_realize (widget->parent);

      btk_widget_ensure_style (widget);
      
      g_signal_emit (widget, widget_signals[REALIZE], 0);

      btk_widget_real_set_has_tooltip (widget,
				       GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (widget), quark_has_tooltip)),
				       TRUE);

      if (BTK_WIDGET_HAS_SHAPE_MASK (widget))
	{
	  shape_info = g_object_get_qdata (G_OBJECT (widget), quark_shape_info);
	  bdk_window_shape_combine_mask (widget->window,
					 shape_info->shape_mask,
					 shape_info->offset_x,
					 shape_info->offset_y);
	}
      
      shape_info = g_object_get_qdata (G_OBJECT (widget), quark_input_shape_info);
      if (shape_info)
	bdk_window_input_shape_combine_mask (widget->window,
					     shape_info->shape_mask,
					     shape_info->offset_x,
					     shape_info->offset_y);

      mode = btk_widget_get_extension_events (widget);
      if (mode != BDK_EXTENSION_EVENTS_NONE)
        btk_widget_set_extension_events_internal (widget, mode, NULL);
    }
}

/**
 * btk_widget_unrealize:
 * @widget: a #BtkWidget
 *
 * This function is only useful in widget implementations.
 * Causes a widget to be unrealized (frees all BDK resources
 * associated with the widget, such as @widget->window).
 **/
void
btk_widget_unrealize (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (BTK_WIDGET_HAS_SHAPE_MASK (widget))
    btk_widget_shape_combine_mask (widget, NULL, 0, 0);

  if (g_object_get_qdata (G_OBJECT (widget), quark_input_shape_info))
    btk_widget_input_shape_combine_mask (widget, NULL, 0, 0);

  if (btk_widget_get_realized (widget))
    {
      g_object_ref (widget);
      _btk_tooltip_hide (widget);
      g_signal_emit (widget, widget_signals[UNREALIZE], 0);
      btk_widget_set_realized (widget, FALSE);
      btk_widget_set_mapped (widget, FALSE);
      g_object_unref (widget);
    }
}

/*****************************************
 * Draw queueing.
 *****************************************/

/**
 * btk_widget_queue_draw_area:
 * @widget: a #BtkWidget
 * @x: x coordinate of upper-left corner of rectangle to redraw
 * @y: y coordinate of upper-left corner of rectangle to redraw
 * @width: width of rebunnyion to draw
 * @height: height of rebunnyion to draw
 *
 * Invalidates the rectangular area of @widget defined by @x, @y,
 * @width and @height by calling bdk_window_invalidate_rect() on the
 * widget's window and all its child windows. Once the main loop
 * becomes idle (after the current batch of events has been processed,
 * roughly), the window will receive expose events for the union of
 * all rebunnyions that have been invalidated.
 *
 * Normally you would only use this function in widget
 * implementations. You might also use it, or
 * bdk_window_invalidate_rect() directly, to schedule a redraw of a
 * #BtkDrawingArea or some portion thereof.
 *
 * Frequently you can just call bdk_window_invalidate_rect() or
 * bdk_window_invalidate_rebunnyion() instead of this function. Those
 * functions will invalidate only a single window, instead of the
 * widget and all its children.
 *
 * The advantage of adding to the invalidated rebunnyion compared to
 * simply drawing immediately is efficiency; using an invalid rebunnyion
 * ensures that you only have to redraw one time.
 **/
void	   
btk_widget_queue_draw_area (BtkWidget *widget,
			    gint       x,
			    gint       y,
			    gint       width,
 			    gint       height)
{
  BdkRectangle invalid_rect;
  BtkWidget *w;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (!btk_widget_get_realized (widget))
    return;
  
  /* Just return if the widget or one of its ancestors isn't mapped */
  for (w = widget; w != NULL; w = w->parent)
    if (!btk_widget_get_mapped (w))
      return;

  /* Find the correct widget */

  if (btk_widget_get_has_window (widget))
    {
      if (widget->parent)
	{
	  /* Translate widget relative to window-relative */

	  gint wx, wy, wwidth, wheight;
	  
	  bdk_window_get_position (widget->window, &wx, &wy);
	  x -= wx - widget->allocation.x;
	  y -= wy - widget->allocation.y;
	  
	  wwidth = bdk_window_get_width (widget->window);
	  wheight = bdk_window_get_height (widget->window);

	  if (x + width <= 0 || y + height <= 0 ||
	      x >= wwidth || y >= wheight)
	    return;
	  
	  if (x < 0)
	    {
	      width += x;  x = 0;
	    }
	  if (y < 0)
	    {
	      height += y; y = 0;
	    }
	  if (x + width > wwidth)
	    width = wwidth - x;
	  if (y + height > wheight)
	    height = wheight - y;
	}
    }

  invalid_rect.x = x;
  invalid_rect.y = y;
  invalid_rect.width = width;
  invalid_rect.height = height;
  
  bdk_window_invalidate_rect (widget->window, &invalid_rect, TRUE);
}

static void
widget_add_child_draw_rectangle (BtkWidget    *widget,
				 BdkRectangle *rect)
{
  BdkRectangle child_rect;
  
  if (!btk_widget_get_mapped (widget) ||
      widget->window != widget->parent->window)
    return;

  btk_widget_get_draw_rectangle (widget, &child_rect);
  bdk_rectangle_union (rect, &child_rect, rect);
}

static void
btk_widget_get_draw_rectangle (BtkWidget    *widget,
			       BdkRectangle *rect)
{
  if (!btk_widget_get_has_window (widget))
    {
      BtkBorder *draw_border = NULL;

      *rect = widget->allocation;

      btk_widget_style_get (widget,
			    "draw-border", &draw_border,
			    NULL);
      if (draw_border)
	{
	  rect->x -= draw_border->left;
	  rect->y -= draw_border->top;
	  rect->width += draw_border->left + draw_border->right;
	  rect->height += draw_border->top + draw_border->bottom;

          btk_border_free (draw_border);
	}

      if (BTK_IS_CONTAINER (widget))
	btk_container_forall (BTK_CONTAINER (widget),
			      (BtkCallback)widget_add_child_draw_rectangle,
			      rect);
    }
  else
    {
      rect->x = 0;
      rect->y = 0;
      rect->width = widget->allocation.width;
      rect->height = widget->allocation.height;
    }
}

/**
 * btk_widget_queue_draw:
 * @widget: a #BtkWidget
 *
 * Equivalent to calling btk_widget_queue_draw_area() for the
 * entire area of a widget.
 **/
void	   
btk_widget_queue_draw (BtkWidget *widget)
{
  BdkRectangle rect;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));

  btk_widget_get_draw_rectangle (widget, &rect);

  btk_widget_queue_draw_area (widget,
			      rect.x, rect.y,
			      rect.width, rect.height);
}

/* Invalidates the given area (allocation-relative-coordinates)
 * in all of the widget's windows
 */
/**
 * btk_widget_queue_clear_area:
 * @widget: a #BtkWidget
 * @x: x coordinate of upper-left corner of rectangle to redraw
 * @y: y coordinate of upper-left corner of rectangle to redraw
 * @width: width of rebunnyion to draw
 * @height: height of rebunnyion to draw
 * 
 * This function is no longer different from
 * btk_widget_queue_draw_area(), though it once was. Now it just calls
 * btk_widget_queue_draw_area(). Originally
 * btk_widget_queue_clear_area() would force a redraw of the
 * background for %BTK_NO_WINDOW widgets, and
 * btk_widget_queue_draw_area() would not. Now both functions ensure
 * the background will be redrawn.
 * 
 * Deprecated: 2.2: Use btk_widget_queue_draw_area() instead.
 **/
void	   
btk_widget_queue_clear_area (BtkWidget *widget,
			     gint       x,
			     gint       y,
			     gint       width,
			     gint       height)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  btk_widget_queue_draw_area (widget, x, y, width, height);
}

/**
 * btk_widget_queue_clear:
 * @widget: a #BtkWidget
 * 
 * This function does the same as btk_widget_queue_draw().
 *
 * Deprecated: 2.2: Use btk_widget_queue_draw() instead.
 **/
void	   
btk_widget_queue_clear (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  btk_widget_queue_draw (widget);
}

/**
 * btk_widget_queue_resize:
 * @widget: a #BtkWidget
 *
 * This function is only for use in widget implementations.
 * Flags a widget to have its size renegotiated; should
 * be called when a widget for some reason has a new size request.
 * For example, when you change the text in a #BtkLabel, #BtkLabel
 * queues a resize to ensure there's enough space for the new text.
 **/
void
btk_widget_queue_resize (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (btk_widget_get_realized (widget))
    btk_widget_queue_shallow_draw (widget);
      
  _btk_size_group_queue_resize (widget);
}

/**
 * btk_widget_queue_resize_no_redraw:
 * @widget: a #BtkWidget
 *
 * This function works like btk_widget_queue_resize(), 
 * except that the widget is not invalidated.
 *
 * Since: 2.4
 **/
void
btk_widget_queue_resize_no_redraw (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  _btk_size_group_queue_resize (widget);
}

/**
 * btk_widget_draw:
 * @widget: a #BtkWidget
 * @area: area to draw
 *
 * In BTK+ 1.2, this function would immediately render the
 * rebunnyion @area of a widget, by invoking the virtual draw method of a
 * widget. In BTK+ 2.0, the draw method is gone, and instead
 * btk_widget_draw() simply invalidates the specified rebunnyion of the
 * widget, then updates the invalid rebunnyion of the widget immediately.
 * Usually you don't want to update the rebunnyion immediately for
 * performance reasons, so in general btk_widget_queue_draw_area() is
 * a better choice if you want to draw a rebunnyion of a widget.
 **/
void
btk_widget_draw (BtkWidget          *widget,
		 const BdkRectangle *area)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (btk_widget_is_drawable (widget))
    {
      if (area)
        btk_widget_queue_draw_area (widget,
                                    area->x, area->y,
                                    area->width, area->height);
      else
        btk_widget_queue_draw (widget);

      bdk_window_process_updates (widget->window, TRUE);
    }
}

/**
 * btk_widget_size_request:
 * @widget: a #BtkWidget
 * @requisition: a #BtkRequisition to be filled in
 * 
 * This function is typically used when implementing a #BtkContainer
 * subclass.  Obtains the preferred size of a widget. The container
 * uses this information to arrange its child widgets and decide what
 * size allocations to give them with btk_widget_size_allocate().
 *
 * You can also call this function from an application, with some
 * caveats. Most notably, getting a size request requires the widget
 * to be associated with a screen, because font information may be
 * needed. Multihead-aware applications should keep this in mind.
 *
 * Also remember that the size request is not necessarily the size
 * a widget will actually be allocated.
 *
 * See also btk_widget_get_child_requisition().
 **/
void
btk_widget_size_request (BtkWidget	*widget,
			 BtkRequisition *requisition)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

#ifdef G_ENABLE_DEBUG
  if (requisition == &widget->requisition)
    g_warning ("btk_widget_size_request() called on child widget with request equal\n to widget->requisition. btk_widget_set_usize() may not work properly.");
#endif /* G_ENABLE_DEBUG */

  _btk_size_group_compute_requisition (widget, requisition);
}

/**
 * btk_widget_get_child_requisition:
 * @widget: a #BtkWidget
 * @requisition: a #BtkRequisition to be filled in
 * 
 * This function is only for use in widget implementations. Obtains
 * @widget->requisition, unless someone has forced a particular
 * geometry on the widget (e.g. with btk_widget_set_size_request()),
 * in which case it returns that geometry instead of the widget's
 * requisition.
 *
 * This function differs from btk_widget_size_request() in that
 * it retrieves the last size request value from @widget->requisition,
 * while btk_widget_size_request() actually calls the "size_request" method
 * on @widget to compute the size request and fill in @widget->requisition,
 * and only then returns @widget->requisition.
 *
 * Because this function does not call the "size_request" method, it
 * can only be used when you know that @widget->requisition is
 * up-to-date, that is, btk_widget_size_request() has been called
 * since the last time a resize was queued. In general, only container
 * implementations have this information; applications should use
 * btk_widget_size_request().
 **/
void
btk_widget_get_child_requisition (BtkWidget	 *widget,
				  BtkRequisition *requisition)
{
  _btk_size_group_get_child_requisition (widget, requisition);
}

static gboolean
invalidate_predicate (BdkWindow *window,
		      gpointer   data)
{
  gpointer user_data;

  bdk_window_get_user_data (window, &user_data);

  return (user_data == data);
}

/* Invalidate @rebunnyion in widget->window and all children
 * of widget->window owned by widget. @rebunnyion is in the
 * same coordinates as widget->allocation and will be
 * modified by this call.
 */
static void
btk_widget_invalidate_widget_windows (BtkWidget *widget,
				      BdkRebunnyion *rebunnyion)
{
  if (!btk_widget_get_realized (widget))
    return;
  
  if (btk_widget_get_has_window (widget) && widget->parent)
    {
      int x, y;
      
      bdk_window_get_position (widget->window, &x, &y);
      bdk_rebunnyion_offset (rebunnyion, -x, -y);
    }

  bdk_window_invalidate_maybe_recurse (widget->window, rebunnyion,
				       invalidate_predicate, widget);
}

/**
 * btk_widget_queue_shallow_draw:
 * @widget: a #BtkWidget
 *
 * Like btk_widget_queue_draw(), but only windows owned
 * by @widget are invalidated.
 **/
static void
btk_widget_queue_shallow_draw (BtkWidget *widget)
{
  BdkRectangle rect;
  BdkRebunnyion *rebunnyion;
  
  if (!btk_widget_get_realized (widget))
    return;

  btk_widget_get_draw_rectangle (widget, &rect);

  /* get_draw_rectangle() gives us window coordinates, we
   * need to convert to the coordinates that widget->allocation
   * is in.
   */
  if (btk_widget_get_has_window (widget) && widget->parent)
    {
      int wx, wy;
      
      bdk_window_get_position (widget->window, &wx, &wy);
      
      rect.x += wx;
      rect.y += wy;
    }
  
  rebunnyion = bdk_rebunnyion_rectangle (&rect);
  btk_widget_invalidate_widget_windows (widget, rebunnyion);
  bdk_rebunnyion_destroy (rebunnyion);
}

/**
 * btk_widget_size_allocate:
 * @widget: a #BtkWidget
 * @allocation: position and size to be allocated to @widget
 *
 * This function is only used by #BtkContainer subclasses, to assign a size
 * and position to their child widgets. 
 **/
void
btk_widget_size_allocate (BtkWidget	*widget,
			  BtkAllocation *allocation)
{
  BtkWidgetAuxInfo *aux_info;
  BdkRectangle real_allocation;
  BdkRectangle old_allocation;
  gboolean alloc_needed;
  gboolean size_changed;
  gboolean position_changed;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
 
#ifdef G_ENABLE_DEBUG
  if (btk_debug_flags & BTK_DEBUG_GEOMETRY)
    {
      gint depth;
      BtkWidget *parent;
      const gchar *name;

      depth = 0;
      parent = widget;
      while (parent)
	{
	  depth++;
	  parent = btk_widget_get_parent (parent);
	}
      
      name = g_type_name (G_OBJECT_TYPE (G_OBJECT (widget)));
      g_print ("btk_widget_size_allocate: %*s%s %d %d\n", 
	       2 * depth, " ", name, 
	       allocation->width, allocation->height);
    }
#endif /* G_ENABLE_DEBUG */
 
  alloc_needed = BTK_WIDGET_ALLOC_NEEDED (widget);
  if (!BTK_WIDGET_REQUEST_NEEDED (widget))      /* Preserve request/allocate ordering */
    BTK_PRIVATE_UNSET_FLAG (widget, BTK_ALLOC_NEEDED);

  old_allocation = widget->allocation;
  real_allocation = *allocation;
  aux_info =_btk_widget_get_aux_info (widget, FALSE);
  
  if (aux_info)
    {
      if (aux_info->x_set)
	real_allocation.x = aux_info->x;
      if (aux_info->y_set)
	real_allocation.y = aux_info->y;
    }

  if (real_allocation.width < 0 || real_allocation.height < 0)
    {
      g_warning ("btk_widget_size_allocate(): attempt to allocate widget with width %d and height %d",
		 real_allocation.width,
		 real_allocation.height);
    }
  
  real_allocation.width = MAX (real_allocation.width, 1);
  real_allocation.height = MAX (real_allocation.height, 1);

  size_changed = (old_allocation.width != real_allocation.width ||
		  old_allocation.height != real_allocation.height);
  position_changed = (old_allocation.x != real_allocation.x ||
		      old_allocation.y != real_allocation.y);

  if (!alloc_needed && !size_changed && !position_changed)
    return;
  
  g_signal_emit (widget, widget_signals[SIZE_ALLOCATE], 0, &real_allocation);

  if (btk_widget_get_mapped (widget))
    {
      if (!btk_widget_get_has_window (widget) && BTK_WIDGET_REDRAW_ON_ALLOC (widget) && position_changed)
	{
	  /* Invalidate union(old_allaction,widget->allocation) in widget->window
	   */
	  BdkRebunnyion *invalidate = bdk_rebunnyion_rectangle (&widget->allocation);
	  bdk_rebunnyion_union_with_rect (invalidate, &old_allocation);

	  bdk_window_invalidate_rebunnyion (widget->window, invalidate, FALSE);
	  bdk_rebunnyion_destroy (invalidate);
	}
      
      if (size_changed)
	{
	  if (BTK_WIDGET_REDRAW_ON_ALLOC (widget))
	    {
	      /* Invalidate union(old_allaction,widget->allocation) in widget->window and descendents owned by widget
	       */
	      BdkRebunnyion *invalidate = bdk_rebunnyion_rectangle (&widget->allocation);
	      bdk_rebunnyion_union_with_rect (invalidate, &old_allocation);

	      btk_widget_invalidate_widget_windows (widget, invalidate);
	      bdk_rebunnyion_destroy (invalidate);
	    }
	}
    }

  if ((size_changed || position_changed) && widget->parent &&
      btk_widget_get_realized (widget->parent) && BTK_CONTAINER (widget->parent)->reallocate_redraws)
    {
      BdkRebunnyion *invalidate = bdk_rebunnyion_rectangle (&widget->parent->allocation);
      btk_widget_invalidate_widget_windows (widget->parent, invalidate);
      bdk_rebunnyion_destroy (invalidate);
    }
}

/**
 * btk_widget_common_ancestor:
 * @widget_a: a #BtkWidget
 * @widget_b: a #BtkWidget
 * 
 * Find the common ancestor of @widget_a and @widget_b that
 * is closest to the two widgets.
 * 
 * Return value: the closest common ancestor of @widget_a and
 *   @widget_b or %NULL if @widget_a and @widget_b do not
 *   share a common ancestor.
 **/
static BtkWidget *
btk_widget_common_ancestor (BtkWidget *widget_a,
			    BtkWidget *widget_b)
{
  BtkWidget *parent_a;
  BtkWidget *parent_b;
  gint depth_a = 0;
  gint depth_b = 0;

  parent_a = widget_a;
  while (parent_a->parent)
    {
      parent_a = parent_a->parent;
      depth_a++;
    }

  parent_b = widget_b;
  while (parent_b->parent)
    {
      parent_b = parent_b->parent;
      depth_b++;
    }

  if (parent_a != parent_b)
    return NULL;

  while (depth_a > depth_b)
    {
      widget_a = widget_a->parent;
      depth_a--;
    }

  while (depth_b > depth_a)
    {
      widget_b = widget_b->parent;
      depth_b--;
    }

  while (widget_a != widget_b)
    {
      widget_a = widget_a->parent;
      widget_b = widget_b->parent;
    }

  return widget_a;
}

/**
 * btk_widget_translate_coordinates:
 * @src_widget:  a #BtkWidget
 * @dest_widget: a #BtkWidget
 * @src_x: X position relative to @src_widget
 * @src_y: Y position relative to @src_widget
 * @dest_x: (out): location to store X position relative to @dest_widget
 * @dest_y: (out): location to store Y position relative to @dest_widget
 *
 * Translate coordinates relative to @src_widget's allocation to coordinates
 * relative to @dest_widget's allocations. In order to perform this
 * operation, both widgets must be realized, and must share a common
 * toplevel.
 * 
 * Return value: %FALSE if either widget was not realized, or there
 *   was no common ancestor. In this case, nothing is stored in
 *   *@dest_x and *@dest_y. Otherwise %TRUE.
 **/
gboolean
btk_widget_translate_coordinates (BtkWidget  *src_widget,
				  BtkWidget  *dest_widget,
				  gint        src_x,
				  gint        src_y,
				  gint       *dest_x,
				  gint       *dest_y)
{
  BtkWidget *ancestor;
  BdkWindow *window;
  GList *dest_list = NULL;

  g_return_val_if_fail (BTK_IS_WIDGET (src_widget), FALSE);
  g_return_val_if_fail (BTK_IS_WIDGET (dest_widget), FALSE);

  ancestor = btk_widget_common_ancestor (src_widget, dest_widget);
  if (!ancestor || !btk_widget_get_realized (src_widget) || !btk_widget_get_realized (dest_widget))
    return FALSE;

  /* Translate from allocation relative to window relative */
  if (btk_widget_get_has_window (src_widget) && src_widget->parent)
    {
      gint wx, wy;
      bdk_window_get_position (src_widget->window, &wx, &wy);

      src_x -= wx - src_widget->allocation.x;
      src_y -= wy - src_widget->allocation.y;
    }
  else
    {
      src_x += src_widget->allocation.x;
      src_y += src_widget->allocation.y;
    }

  /* Translate to the common ancestor */
  window = src_widget->window;
  while (window != ancestor->window)
    {
      gdouble dx, dy;

      bdk_window_coords_to_parent (window, src_x, src_y, &dx, &dy);

      src_x = dx;
      src_y = dy;

      window = bdk_window_get_effective_parent (window);

      if (!window)		/* Handle BtkHandleBox */
	return FALSE;
    }

  /* And back */
  window = dest_widget->window;
  while (window != ancestor->window)
    {
      dest_list = g_list_prepend (dest_list, window);

      window = bdk_window_get_effective_parent (window);

      if (!window)		/* Handle BtkHandleBox */
        {
          g_list_free (dest_list);
          return FALSE;
        }
    }

  while (dest_list)
    {
      gdouble dx, dy;

      bdk_window_coords_from_parent (dest_list->data, src_x, src_y, &dx, &dy);

      src_x = dx;
      src_y = dy;

      dest_list = g_list_remove (dest_list, dest_list->data);
    }

  /* Translate from window relative to allocation relative */
  if (btk_widget_get_has_window (dest_widget) && dest_widget->parent)
    {
      gint wx, wy;
      bdk_window_get_position (dest_widget->window, &wx, &wy);

      src_x += wx - dest_widget->allocation.x;
      src_y += wy - dest_widget->allocation.y;
    }
  else
    {
      src_x -= dest_widget->allocation.x;
      src_y -= dest_widget->allocation.y;
    }

  if (dest_x)
    *dest_x = src_x;
  if (dest_y)
    *dest_y = src_y;

  return TRUE;
}

static void
btk_widget_real_size_allocate (BtkWidget     *widget,
			       BtkAllocation *allocation)
{
  widget->allocation = *allocation;
  
  if (btk_widget_get_realized (widget) &&
      btk_widget_get_has_window (widget))
     {
	bdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);
     }
}

static gboolean
btk_widget_real_can_activate_accel (BtkWidget *widget,
                                    guint      signal_id)
{
  /* widgets must be onscreen for accels to take effect */
  return btk_widget_is_sensitive (widget) &&
         btk_widget_is_drawable (widget) &&
         bdk_window_is_viewable (widget->window);
}

/**
 * btk_widget_can_activate_accel:
 * @widget: a #BtkWidget
 * @signal_id: the ID of a signal installed on @widget
 * 
 * Determines whether an accelerator that activates the signal
 * identified by @signal_id can currently be activated.
 * This is done by emitting the #BtkWidget::can-activate-accel
 * signal on @widget; if the signal isn't overridden by a
 * handler or in a derived widget, then the default check is
 * that the widget must be sensitive, and the widget and all
 * its ancestors mapped.
 *
 * Return value: %TRUE if the accelerator can be activated.
 *
 * Since: 2.4
 **/
gboolean
btk_widget_can_activate_accel (BtkWidget *widget,
                               guint      signal_id)
{
  gboolean can_activate = FALSE;
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_signal_emit (widget, widget_signals[CAN_ACTIVATE_ACCEL], 0, signal_id, &can_activate);
  return can_activate;
}

typedef struct {
  GClosure   closure;
  guint      signal_id;
} AccelClosure;

static void
closure_accel_activate (GClosure     *closure,
			GValue       *return_value,
			guint         n_param_values,
			const GValue *param_values,
			gpointer      invocation_hint,
			gpointer      marshal_data)
{
  AccelClosure *aclosure = (AccelClosure*) closure;
  gboolean can_activate = btk_widget_can_activate_accel (closure->data, aclosure->signal_id);

  if (can_activate)
    g_signal_emit (closure->data, aclosure->signal_id, 0);

  /* whether accelerator was handled */
  g_value_set_boolean (return_value, can_activate);
}

static void
closures_destroy (gpointer data)
{
  GSList *slist, *closures = data;

  for (slist = closures; slist; slist = slist->next)
    {
      g_closure_invalidate (slist->data);
      g_closure_unref (slist->data);
    }
  g_slist_free (closures);
}

static GClosure*
widget_new_accel_closure (BtkWidget *widget,
			  guint      signal_id)
{
  AccelClosure *aclosure;
  GClosure *closure = NULL;
  GSList *slist, *closures;

  closures = g_object_steal_qdata (G_OBJECT (widget), quark_accel_closures);
  for (slist = closures; slist; slist = slist->next)
    if (!btk_accel_group_from_accel_closure (slist->data))
      {
	/* reuse this closure */
	closure = slist->data;
	break;
      }
  if (!closure)
    {
      closure = g_closure_new_object (sizeof (AccelClosure), G_OBJECT (widget));
      closures = g_slist_prepend (closures, g_closure_ref (closure));
      g_closure_sink (closure);
      g_closure_set_marshal (closure, closure_accel_activate);
    }
  g_object_set_qdata_full (G_OBJECT (widget), quark_accel_closures, closures, closures_destroy);
  
  aclosure = (AccelClosure*) closure;
  g_assert (closure->data == widget);
  g_assert (closure->marshal == closure_accel_activate);
  aclosure->signal_id = signal_id;

  return closure;
}

/**
 * btk_widget_add_accelerator
 * @widget:       widget to install an accelerator on
 * @accel_signal: widget signal to emit on accelerator activation
 * @accel_group:  accel group for this widget, added to its toplevel
 * @accel_key:    BDK keyval of the accelerator
 * @accel_mods:   modifier key combination of the accelerator
 * @accel_flags:  flag accelerators, e.g. %BTK_ACCEL_VISIBLE
 *
 * Installs an accelerator for this @widget in @accel_group that causes
 * @accel_signal to be emitted if the accelerator is activated.
 * The @accel_group needs to be added to the widget's toplevel via
 * btk_window_add_accel_group(), and the signal must be of type %G_RUN_ACTION.
 * Accelerators added through this function are not user changeable during
 * runtime. If you want to support accelerators that can be changed by the
 * user, use btk_accel_map_add_entry() and btk_widget_set_accel_path() or
 * btk_menu_item_set_accel_path() instead.
 */
void
btk_widget_add_accelerator (BtkWidget      *widget,
			    const gchar    *accel_signal,
			    BtkAccelGroup  *accel_group,
			    guint           accel_key,
			    BdkModifierType accel_mods,
			    BtkAccelFlags   accel_flags)
{
  GClosure *closure;
  GSignalQuery query;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (accel_signal != NULL);
  g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));

  g_signal_query (g_signal_lookup (accel_signal, G_OBJECT_TYPE (widget)), &query);
  if (!query.signal_id ||
      !(query.signal_flags & G_SIGNAL_ACTION) ||
      query.return_type != G_TYPE_NONE ||
      query.n_params)
    {
      /* hmm, should be elaborate enough */
      g_warning (B_STRLOC ": widget `%s' has no activatable signal \"%s\" without arguments",
		 G_OBJECT_TYPE_NAME (widget), accel_signal);
      return;
    }

  closure = widget_new_accel_closure (widget, query.signal_id);

  g_object_ref (widget);

  /* install the accelerator. since we don't map this onto an accel_path,
   * the accelerator will automatically be locked.
   */
  btk_accel_group_connect (accel_group,
			   accel_key,
			   accel_mods,
			   accel_flags | BTK_ACCEL_LOCKED,
			   closure);

  g_signal_emit (widget, widget_signals[ACCEL_CLOSURES_CHANGED], 0);

  g_object_unref (widget);
}

/**
 * btk_widget_remove_accelerator:
 * @widget:       widget to install an accelerator on
 * @accel_group:  accel group for this widget
 * @accel_key:    BDK keyval of the accelerator
 * @accel_mods:   modifier key combination of the accelerator
 * @returns:      whether an accelerator was installed and could be removed
 *
 * Removes an accelerator from @widget, previously installed with
 * btk_widget_add_accelerator().
 */
gboolean
btk_widget_remove_accelerator (BtkWidget      *widget,
			       BtkAccelGroup  *accel_group,
			       guint           accel_key,
			       BdkModifierType accel_mods)
{
  BtkAccelGroupEntry *ag_entry;
  GList *slist, *clist;
  guint n;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (BTK_IS_ACCEL_GROUP (accel_group), FALSE);

  ag_entry = btk_accel_group_query (accel_group, accel_key, accel_mods, &n);
  clist = btk_widget_list_accel_closures (widget);
  for (slist = clist; slist; slist = slist->next)
    {
      guint i;

      for (i = 0; i < n; i++)
	if (slist->data == (gpointer) ag_entry[i].closure)
	  {
	    gboolean is_removed = btk_accel_group_disconnect (accel_group, slist->data);

	    g_signal_emit (widget, widget_signals[ACCEL_CLOSURES_CHANGED], 0);

	    g_list_free (clist);

	    return is_removed;
	  }
    }
  g_list_free (clist);

  g_warning (B_STRLOC ": no accelerator (%u,%u) installed in accel group (%p) for %s (%p)",
	     accel_key, accel_mods, accel_group,
	     G_OBJECT_TYPE_NAME (widget), widget);

  return FALSE;
}

/**
 * btk_widget_list_accel_closures:
 * @widget:  widget to list accelerator closures for
 *
 * Lists the closures used by @widget for accelerator group connections
 * with btk_accel_group_connect_by_path() or btk_accel_group_connect().
 * The closures can be used to monitor accelerator changes on @widget,
 * by connecting to the @BtkAccelGroup::accel-changed signal of the
 * #BtkAccelGroup of a closure which can be found out with
 * btk_accel_group_from_accel_closure().
 *
 * Return value: (transfer container) (element-type GClosure):
 *     a newly allocated #GList of closures
 */
GList*
btk_widget_list_accel_closures (BtkWidget *widget)
{
  GSList *slist;
  GList *clist = NULL;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  for (slist = g_object_get_qdata (G_OBJECT (widget), quark_accel_closures); slist; slist = slist->next)
    if (btk_accel_group_from_accel_closure (slist->data))
      clist = g_list_prepend (clist, slist->data);
  return clist;
}

typedef struct {
  GQuark         path_quark;
  BtkAccelGroup *accel_group;
  GClosure      *closure;
} AccelPath;

static void
destroy_accel_path (gpointer data)
{
  AccelPath *apath = data;

  btk_accel_group_disconnect (apath->accel_group, apath->closure);

  /* closures_destroy takes care of unrefing the closure */
  g_object_unref (apath->accel_group);
  
  g_slice_free (AccelPath, apath);
}


/**
 * btk_widget_set_accel_path:
 * @widget: a #BtkWidget
 * @accel_path: (allow-none): path used to look up the accelerator
 * @accel_group: (allow-none): a #BtkAccelGroup.
 *
 * Given an accelerator group, @accel_group, and an accelerator path,
 * @accel_path, sets up an accelerator in @accel_group so whenever the
 * key binding that is defined for @accel_path is pressed, @widget
 * will be activated.  This removes any accelerators (for any
 * accelerator group) installed by previous calls to
 * btk_widget_set_accel_path(). Associating accelerators with
 * paths allows them to be modified by the user and the modifications
 * to be saved for future use. (See btk_accel_map_save().)
 *
 * This function is a low level function that would most likely
 * be used by a menu creation system like #BtkUIManager. If you
 * use #BtkUIManager, setting up accelerator paths will be done
 * automatically.
 *
 * Even when you you aren't using #BtkUIManager, if you only want to
 * set up accelerators on menu items btk_menu_item_set_accel_path()
 * provides a somewhat more convenient interface.
 * 
 * Note that @accel_path string will be stored in a #GQuark. Therefore, if you
 * pass a static string, you can save some memory by interning it first with 
 * g_intern_static_string().
 **/
void
btk_widget_set_accel_path (BtkWidget     *widget,
			   const gchar   *accel_path,
			   BtkAccelGroup *accel_group)
{
  AccelPath *apath;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_WIDGET_GET_CLASS (widget)->activate_signal != 0);

  if (accel_path)
    {
      g_return_if_fail (BTK_IS_ACCEL_GROUP (accel_group));
      g_return_if_fail (_btk_accel_path_is_valid (accel_path));

      btk_accel_map_add_entry (accel_path, 0, 0);
      apath = g_slice_new (AccelPath);
      apath->accel_group = g_object_ref (accel_group);
      apath->path_quark = g_quark_from_string (accel_path);
      apath->closure = widget_new_accel_closure (widget, BTK_WIDGET_GET_CLASS (widget)->activate_signal);
    }
  else
    apath = NULL;

  /* also removes possible old settings */
  g_object_set_qdata_full (G_OBJECT (widget), quark_accel_path, apath, destroy_accel_path);

  if (apath)
    btk_accel_group_connect_by_path (apath->accel_group, g_quark_to_string (apath->path_quark), apath->closure);

  g_signal_emit (widget, widget_signals[ACCEL_CLOSURES_CHANGED], 0);
}

const gchar*
_btk_widget_get_accel_path (BtkWidget *widget,
			    gboolean  *locked)
{
  AccelPath *apath;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  apath = g_object_get_qdata (G_OBJECT (widget), quark_accel_path);
  if (locked)
    *locked = apath ? apath->accel_group->lock_count > 0 : TRUE;
  return apath ? g_quark_to_string (apath->path_quark) : NULL;
}

/**
 * btk_widget_mnemonic_activate:
 * @widget: a #BtkWidget
 * @group_cycling:  %TRUE if there are other widgets with the same mnemonic
 *
 * Emits the #BtkWidget::mnemonic-activate signal.
 * 
 * The default handler for this signal activates the @widget if
 * @group_cycling is %FALSE, and just grabs the focus if @group_cycling
 * is %TRUE.
 *
 * Returns: %TRUE if the signal has been handled
 */
gboolean
btk_widget_mnemonic_activate (BtkWidget *widget,
                              gboolean   group_cycling)
{
  gboolean handled;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  group_cycling = group_cycling != FALSE;
  if (!btk_widget_is_sensitive (widget))
    handled = TRUE;
  else
    g_signal_emit (widget,
		   widget_signals[MNEMONIC_ACTIVATE],
		   0,
		   group_cycling,
		   &handled);
  return handled;
}

static gboolean
btk_widget_real_mnemonic_activate (BtkWidget *widget,
                                   gboolean   group_cycling)
{
  if (!group_cycling && BTK_WIDGET_GET_CLASS (widget)->activate_signal)
    btk_widget_activate (widget);
  else if (btk_widget_get_can_focus (widget))
    btk_widget_grab_focus (widget);
  else
    {
      g_warning ("widget `%s' isn't suitable for mnemonic activation",
		 G_OBJECT_TYPE_NAME (widget));
      btk_widget_error_bell (widget);
    }
  return TRUE;
}

static gboolean
btk_widget_real_key_press_event (BtkWidget         *widget,
				 BdkEventKey       *event)
{
  return btk_bindings_activate_event (BTK_OBJECT (widget), event);
}

static gboolean
btk_widget_real_key_release_event (BtkWidget         *widget,
				   BdkEventKey       *event)
{
  return btk_bindings_activate_event (BTK_OBJECT (widget), event);
}

static gboolean
btk_widget_real_focus_in_event (BtkWidget     *widget,
                                BdkEventFocus *event)
{
  btk_widget_queue_shallow_draw (widget);

  return FALSE;
}

static gboolean
btk_widget_real_focus_out_event (BtkWidget     *widget,
                                 BdkEventFocus *event)
{
  btk_widget_queue_shallow_draw (widget);

  return FALSE;
}

#define WIDGET_REALIZED_FOR_EVENT(widget, event) \
     (event->type == BDK_FOCUS_CHANGE || btk_widget_get_realized(widget))

/**
 * btk_widget_event:
 * @widget: a #BtkWidget
 * @event: a #BdkEvent
 * 
 * Rarely-used function. This function is used to emit
 * the event signals on a widget (those signals should never
 * be emitted without using this function to do so).
 * If you want to synthesize an event though, don't use this function;
 * instead, use btk_main_do_event() so the event will behave as if
 * it were in the event queue. Don't synthesize expose events; instead,
 * use bdk_window_invalidate_rect() to invalidate a rebunnyion of the
 * window.
 * 
 * Return value: return from the event signal emission (%TRUE if 
 *               the event was handled)
 **/
gboolean
btk_widget_event (BtkWidget *widget,
		  BdkEvent  *event)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), TRUE);
  g_return_val_if_fail (WIDGET_REALIZED_FOR_EVENT (widget, event), TRUE);

  if (event->type == BDK_EXPOSE)
    {
      g_warning ("Events of type BDK_EXPOSE cannot be synthesized. To get "
		 "the same effect, call bdk_window_invalidate_rect/rebunnyion(), "
		 "followed by bdk_window_process_updates().");
      return TRUE;
    }
  
  return btk_widget_event_internal (widget, event);
}


/**
 * btk_widget_send_expose:
 * @widget: a #BtkWidget
 * @event: a expose #BdkEvent
 * 
 * Very rarely-used function. This function is used to emit
 * an expose event signals on a widget. This function is not
 * normally used directly. The only time it is used is when
 * propagating an expose event to a child %NO_WINDOW widget, and
 * that is normally done using btk_container_propagate_expose().
 *
 * If you want to force an area of a window to be redrawn, 
 * use bdk_window_invalidate_rect() or bdk_window_invalidate_rebunnyion().
 * To cause the redraw to be done immediately, follow that call
 * with a call to bdk_window_process_updates().
 * 
 * Return value: return from the event signal emission (%TRUE if 
 *               the event was handled)
 **/
gint
btk_widget_send_expose (BtkWidget *widget,
			BdkEvent  *event)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), TRUE);
  g_return_val_if_fail (btk_widget_get_realized (widget), TRUE);
  g_return_val_if_fail (event != NULL, TRUE);
  g_return_val_if_fail (event->type == BDK_EXPOSE, TRUE);

  return btk_widget_event_internal (widget, event);
}

static gboolean
event_window_is_still_viewable (BdkEvent *event)
{
  /* Some programs, such as bunny-theme-manager, fake widgets
   * into exposing onto a pixmap by sending expose events with
   * event->window pointing to a pixmap
   */
  if (BDK_IS_PIXMAP (event->any.window))
    return event->type == BDK_EXPOSE;
  
  /* Check that we think the event's window is viewable before
   * delivering the event, to prevent suprises. We do this here
   * at the last moment, since the event may have been queued
   * up behind other events, held over a recursive main loop, etc.
   */
  switch (event->type)
    {
    case BDK_EXPOSE:
    case BDK_MOTION_NOTIFY:
    case BDK_BUTTON_PRESS:
    case BDK_2BUTTON_PRESS:
    case BDK_3BUTTON_PRESS:
    case BDK_KEY_PRESS:
    case BDK_ENTER_NOTIFY:
    case BDK_PROXIMITY_IN:
    case BDK_SCROLL:
      return event->any.window && bdk_window_is_viewable (event->any.window);

#if 0
    /* The following events are the second half of paired events;
     * we always deliver them to deal with widgets that clean up
     * on the second half.
     */
    case BDK_BUTTON_RELEASE:
    case BDK_KEY_RELEASE:
    case BDK_LEAVE_NOTIFY:
    case BDK_PROXIMITY_OUT:
#endif      
      
    default:
      /* Remaining events would make sense on an not-viewable window,
       * or don't have an associated window.
       */
      return TRUE;
    }
}

static gint
btk_widget_event_internal (BtkWidget *widget,
			   BdkEvent  *event)
{
  gboolean return_val = FALSE;

  /* We check only once for is-still-visible; if someone
   * hides the window in on of the signals on the widget,
   * they are responsible for returning TRUE to terminate
   * handling.
   */
  if (!event_window_is_still_viewable (event))
    return TRUE;

  g_object_ref (widget);

  g_signal_emit (widget, widget_signals[EVENT], 0, event, &return_val);
  return_val |= !WIDGET_REALIZED_FOR_EVENT (widget, event);
  if (!return_val)
    {
      gint signal_num;

      switch (event->type)
	{
	case BDK_NOTHING:
	  signal_num = -1;
	  break;
	case BDK_BUTTON_PRESS:
	case BDK_2BUTTON_PRESS:
	case BDK_3BUTTON_PRESS:
	  signal_num = BUTTON_PRESS_EVENT;
	  break;
	case BDK_SCROLL:
	  signal_num = SCROLL_EVENT;
	  break;
	case BDK_BUTTON_RELEASE:
	  signal_num = BUTTON_RELEASE_EVENT;
	  break;
	case BDK_MOTION_NOTIFY:
	  signal_num = MOTION_NOTIFY_EVENT;
	  break;
	case BDK_DELETE:
	  signal_num = DELETE_EVENT;
	  break;
	case BDK_DESTROY:
	  signal_num = DESTROY_EVENT;
	  _btk_tooltip_hide (widget);
	  break;
	case BDK_KEY_PRESS:
	  signal_num = KEY_PRESS_EVENT;
	  break;
	case BDK_KEY_RELEASE:
	  signal_num = KEY_RELEASE_EVENT;
	  break;
	case BDK_ENTER_NOTIFY:
	  signal_num = ENTER_NOTIFY_EVENT;
	  break;
	case BDK_LEAVE_NOTIFY:
	  signal_num = LEAVE_NOTIFY_EVENT;
	  break;
	case BDK_FOCUS_CHANGE:
	  signal_num = event->focus_change.in ? FOCUS_IN_EVENT : FOCUS_OUT_EVENT;
	  if (event->focus_change.in)
	    _btk_tooltip_focus_in (widget);
	  else
	    _btk_tooltip_focus_out (widget);
	  break;
	case BDK_CONFIGURE:
	  signal_num = CONFIGURE_EVENT;
	  break;
	case BDK_MAP:
	  signal_num = MAP_EVENT;
	  break;
	case BDK_UNMAP:
	  signal_num = UNMAP_EVENT;
	  break;
	case BDK_WINDOW_STATE:
	  signal_num = WINDOW_STATE_EVENT;
	  break;
	case BDK_PROPERTY_NOTIFY:
	  signal_num = PROPERTY_NOTIFY_EVENT;
	  break;
	case BDK_SELECTION_CLEAR:
	  signal_num = SELECTION_CLEAR_EVENT;
	  break;
	case BDK_SELECTION_REQUEST:
	  signal_num = SELECTION_REQUEST_EVENT;
	  break;
	case BDK_SELECTION_NOTIFY:
	  signal_num = SELECTION_NOTIFY_EVENT;
	  break;
	case BDK_PROXIMITY_IN:
	  signal_num = PROXIMITY_IN_EVENT;
	  break;
	case BDK_PROXIMITY_OUT:
	  signal_num = PROXIMITY_OUT_EVENT;
	  break;
	case BDK_NO_EXPOSE:
	  signal_num = NO_EXPOSE_EVENT;
	  break;
	case BDK_CLIENT_EVENT:
	  signal_num = CLIENT_EVENT;
	  break;
	case BDK_EXPOSE:
	  signal_num = EXPOSE_EVENT;
	  break;
	case BDK_VISIBILITY_NOTIFY:
	  signal_num = VISIBILITY_NOTIFY_EVENT;
	  break;
	case BDK_GRAB_BROKEN:
	  signal_num = GRAB_BROKEN;
	  break;
	case BDK_DAMAGE:
	  signal_num = DAMAGE_EVENT;
	  break;
	default:
	  g_warning ("btk_widget_event(): unhandled event type: %d", event->type);
	  signal_num = -1;
	  break;
	}
      if (signal_num != -1)
	g_signal_emit (widget, widget_signals[signal_num], 0, event, &return_val);
    }
  if (WIDGET_REALIZED_FOR_EVENT (widget, event))
    g_signal_emit (widget, widget_signals[EVENT_AFTER], 0, event);
  else
    return_val = TRUE;

  g_object_unref (widget);

  return return_val;
}

/**
 * btk_widget_activate:
 * @widget: a #BtkWidget that's activatable
 * 
 * For widgets that can be "activated" (buttons, menu items, etc.)
 * this function activates them. Activation is what happens when you
 * press Enter on a widget during key navigation. If @widget isn't 
 * activatable, the function returns %FALSE.
 * 
 * Return value: %TRUE if the widget was activatable
 **/
gboolean
btk_widget_activate (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  
  if (WIDGET_CLASS (widget)->activate_signal)
    {
      /* FIXME: we should eventually check the signals signature here */
      g_signal_emit (widget, WIDGET_CLASS (widget)->activate_signal, 0);

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * btk_widget_set_scroll_adjustments:
 * @widget: a #BtkWidget
 * @hadjustment: (allow-none): an adjustment for horizontal scrolling, or %NULL
 * @vadjustment: (allow-none): an adjustment for vertical scrolling, or %NULL
 *
 * For widgets that support scrolling, sets the scroll adjustments and
 * returns %TRUE.  For widgets that don't support scrolling, does
 * nothing and returns %FALSE. Widgets that don't support scrolling
 * can be scrolled by placing them in a #BtkViewport, which does
 * support scrolling.
 * 
 * Return value: %TRUE if the widget supports scrolling
 **/
gboolean
btk_widget_set_scroll_adjustments (BtkWidget     *widget,
				   BtkAdjustment *hadjustment,
				   BtkAdjustment *vadjustment)
{
  guint signal_id;
  GSignalQuery query;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  if (hadjustment)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (hadjustment), FALSE);
  if (vadjustment)
    g_return_val_if_fail (BTK_IS_ADJUSTMENT (vadjustment), FALSE);

  signal_id = WIDGET_CLASS (widget)->set_scroll_adjustments_signal;
  if (!signal_id)
    return FALSE;

  g_signal_query (signal_id, &query);
  if (!query.signal_id ||
      !g_type_is_a (query.itype, BTK_TYPE_WIDGET) ||
      query.return_type != G_TYPE_NONE ||
      query.n_params != 2 ||
      query.param_types[0] != BTK_TYPE_ADJUSTMENT ||
      query.param_types[1] != BTK_TYPE_ADJUSTMENT)
    {
      g_warning (B_STRLOC ": signal \"%s::%s\" has wrong signature",
		 G_OBJECT_TYPE_NAME (widget), query.signal_name);
      return FALSE;
    }
      
  g_signal_emit (widget, signal_id, 0, hadjustment, vadjustment);
  return TRUE;
}

static void
btk_widget_reparent_subwindows (BtkWidget *widget,
				BdkWindow *new_window)
{
  if (!btk_widget_get_has_window (widget))
    {
      GList *children = bdk_window_get_children (widget->window);
      GList *tmp_list;

      for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
	{
	  BdkWindow *window = tmp_list->data;
	  gpointer child;

	  bdk_window_get_user_data (window, &child);
	  while (child && child != widget)
	    child = ((BtkWidget*) child)->parent;

	  if (child)
	    bdk_window_reparent (window, new_window, 0, 0);
	}

      g_list_free (children);
    }
  else
   {
     BdkWindow *parent;
     GList *tmp_list, *children;

     parent = bdk_window_get_parent (widget->window);

     if (parent == NULL)
       bdk_window_reparent (widget->window, new_window, 0, 0);
     else
       {
	 children = bdk_window_get_children (parent);
	 
	 for (tmp_list = children; tmp_list; tmp_list = tmp_list->next)
	   {
	     BdkWindow *window = tmp_list->data;
	     gpointer child;

	     bdk_window_get_user_data (window, &child);

	     if (child == widget)
	       bdk_window_reparent (window, new_window, 0, 0);
	   }
	 
	 g_list_free (children);
       }
   }
}

static void
btk_widget_reparent_fixup_child (BtkWidget *widget,
				 gpointer   client_data)
{
  g_assert (client_data != NULL);
  
  if (!btk_widget_get_has_window (widget))
    {
      if (widget->window)
	g_object_unref (widget->window);
      widget->window = (BdkWindow*) client_data;
      if (widget->window)
	g_object_ref (widget->window);

      if (BTK_IS_CONTAINER (widget))
        btk_container_forall (BTK_CONTAINER (widget),
                              btk_widget_reparent_fixup_child,
                              client_data);
    }
}

/**
 * btk_widget_reparent:
 * @widget: a #BtkWidget
 * @new_parent: a #BtkContainer to move the widget into
 *
 * Moves a widget from one #BtkContainer to another, handling reference
 * count issues to avoid destroying the widget.
 **/
void
btk_widget_reparent (BtkWidget *widget,
		     BtkWidget *new_parent)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_IS_CONTAINER (new_parent));
  g_return_if_fail (widget->parent != NULL);
  
  if (widget->parent != new_parent)
    {
      /* First try to see if we can get away without unrealizing
       * the widget as we reparent it. if so we set a flag so
       * that btk_widget_unparent doesn't unrealize widget
       */
      if (btk_widget_get_realized (widget) && btk_widget_get_realized (new_parent))
	BTK_PRIVATE_SET_FLAG (widget, BTK_IN_REPARENT);
      
      g_object_ref (widget);
      btk_container_remove (BTK_CONTAINER (widget->parent), widget);
      btk_container_add (BTK_CONTAINER (new_parent), widget);
      g_object_unref (widget);
      
      if (BTK_WIDGET_IN_REPARENT (widget))
	{
	  BTK_PRIVATE_UNSET_FLAG (widget, BTK_IN_REPARENT);

	  btk_widget_reparent_subwindows (widget, btk_widget_get_parent_window (widget));
	  btk_widget_reparent_fixup_child (widget,
					   btk_widget_get_parent_window (widget));
	}

      g_object_notify (G_OBJECT (widget), "parent");
    }
}

/**
 * btk_widget_intersect:
 * @widget: a #BtkWidget
 * @area: a rectangle
 * @intersection: rectangle to store intersection of @widget and @area
 * 
 * Computes the intersection of a @widget's area and @area, storing
 * the intersection in @intersection, and returns %TRUE if there was
 * an intersection.  @intersection may be %NULL if you're only
 * interested in whether there was an intersection.
 * 
 * Return value: %TRUE if there was an intersection
 **/
gboolean
btk_widget_intersect (BtkWidget	         *widget,
		      const BdkRectangle *area,
		      BdkRectangle       *intersection)
{
  BdkRectangle *dest;
  BdkRectangle tmp;
  gint return_val;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (area != NULL, FALSE);
  
  if (intersection)
    dest = intersection;
  else
    dest = &tmp;
  
  return_val = bdk_rectangle_intersect (&widget->allocation, area, dest);
  
  if (return_val && intersection && btk_widget_get_has_window (widget))
    {
      intersection->x -= widget->allocation.x;
      intersection->y -= widget->allocation.y;
    }
  
  return return_val;
}

/**
 * btk_widget_rebunnyion_intersect:
 * @widget: a #BtkWidget
 * @rebunnyion: a #BdkRebunnyion, in the same coordinate system as 
 *          @widget->allocation. That is, relative to @widget->window
 *          for %NO_WINDOW widgets; relative to the parent window
 *          of @widget->window for widgets with their own window.
 * @returns: A newly allocated rebunnyion holding the intersection of @widget
 *           and @rebunnyion. The coordinates of the return value are
 *           relative to @widget->window for %NO_WINDOW widgets, and
 *           relative to the parent window of @widget->window for
 *           widgets with their own window.
 * 
 * Computes the intersection of a @widget's area and @rebunnyion, returning
 * the intersection. The result may be empty, use bdk_rebunnyion_empty() to
 * check.
 **/
BdkRebunnyion *
btk_widget_rebunnyion_intersect (BtkWidget       *widget,
			     const BdkRebunnyion *rebunnyion)
{
  BdkRectangle rect;
  BdkRebunnyion *dest;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (rebunnyion != NULL, NULL);

  btk_widget_get_draw_rectangle (widget, &rect);
  
  dest = bdk_rebunnyion_rectangle (&rect);
 
  bdk_rebunnyion_intersect (dest, rebunnyion);

  return dest;
}

/**
 * _btk_widget_grab_notify:
 * @widget: a #BtkWidget
 * @was_grabbed: whether a grab is now in effect
 * 
 * Emits the #BtkWidget::grab-notify signal on @widget.
 * 
 * Since: 2.6
 **/
void
_btk_widget_grab_notify (BtkWidget *widget,
			 gboolean   was_grabbed)
{
  g_signal_emit (widget, widget_signals[GRAB_NOTIFY], 0, was_grabbed);
}

/**
 * btk_widget_grab_focus:
 * @widget: a #BtkWidget
 * 
 * Causes @widget to have the keyboard focus for the #BtkWindow it's
 * inside. @widget must be a focusable widget, such as a #BtkEntry;
 * something like #BtkFrame won't work.
 *
 * More precisely, it must have the %BTK_CAN_FOCUS flag set. Use
 * btk_widget_set_can_focus() to modify that flag.
 *
 * The widget also needs to be realized and mapped. This is indicated by the
 * related signals. Grabbing the focus immediately after creating the widget
 * will likely fail and cause critical warnings.
 **/
void
btk_widget_grab_focus (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (!btk_widget_is_sensitive (widget))
    return;
  
  g_object_ref (widget);
  g_signal_emit (widget, widget_signals[GRAB_FOCUS], 0);
  g_object_notify (G_OBJECT (widget), "has-focus");
  g_object_unref (widget);
}

static void
reset_focus_recurse (BtkWidget *widget,
		     gpointer   data)
{
  if (BTK_IS_CONTAINER (widget))
    {
      BtkContainer *container;

      container = BTK_CONTAINER (widget);
      btk_container_set_focus_child (container, NULL);

      btk_container_foreach (container,
			     reset_focus_recurse,
			     NULL);
    }
}

static void
btk_widget_real_grab_focus (BtkWidget *focus_widget)
{
  if (btk_widget_get_can_focus (focus_widget))
    {
      BtkWidget *toplevel;
      BtkWidget *widget;
      
      /* clear the current focus setting, break if the current widget
       * is the focus widget's parent, since containers above that will
       * be set by the next loop.
       */
      toplevel = btk_widget_get_toplevel (focus_widget);
      if (btk_widget_is_toplevel (toplevel) && BTK_IS_WINDOW (toplevel))
	{
	  widget = BTK_WINDOW (toplevel)->focus_widget;
	  
	  if (widget == focus_widget)
	    {
	      /* We call _btk_window_internal_set_focus() here so that the
	       * toplevel window can request the focus if necessary.
	       * This is needed when the toplevel is a BtkPlug
	       */
	      if (!btk_widget_has_focus (widget))
		_btk_window_internal_set_focus (BTK_WINDOW (toplevel), focus_widget);

	      return;
	    }
	  
	  if (widget)
	    {
	      while (widget->parent && widget->parent != focus_widget->parent)
		{
		  widget = widget->parent;
		  btk_container_set_focus_child (BTK_CONTAINER (widget), NULL);
		}
	    }
	}
      else if (toplevel != focus_widget)
	{
	  /* btk_widget_grab_focus() operates on a tree without window...
	   * actually, this is very questionable behaviour.
	   */
	  
	  btk_container_foreach (BTK_CONTAINER (toplevel),
				 reset_focus_recurse,
				 NULL);
	}

      /* now propagate the new focus up the widget tree and finally
       * set it on the window
       */
      widget = focus_widget;
      while (widget->parent)
	{
	  btk_container_set_focus_child (BTK_CONTAINER (widget->parent), widget);
	  widget = widget->parent;
	}
      if (BTK_IS_WINDOW (widget))
	_btk_window_internal_set_focus (BTK_WINDOW (widget), focus_widget);
    }
}

static gboolean
btk_widget_real_query_tooltip (BtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_tip,
			       BtkTooltip *tooltip)
{
  gchar *tooltip_markup;
  gboolean has_tooltip;

  tooltip_markup = g_object_get_qdata (G_OBJECT (widget), quark_tooltip_markup);
  has_tooltip = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (widget), quark_has_tooltip));

  if (has_tooltip && tooltip_markup)
    {
      btk_tooltip_set_markup (tooltip, tooltip_markup);
      return TRUE;
    }

  return FALSE;
}

static gboolean
btk_widget_real_show_help (BtkWidget        *widget,
                           BtkWidgetHelpType help_type)
{
  if (help_type == BTK_WIDGET_HELP_TOOLTIP)
    {
      _btk_tooltip_toggle_keyboard_mode (widget);

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
btk_widget_real_focus (BtkWidget         *widget,
                       BtkDirectionType   direction)
{
  if (!btk_widget_get_can_focus (widget))
    return FALSE;
  
  if (!btk_widget_is_focus (widget))
    {
      btk_widget_grab_focus (widget);
      return TRUE;
    }
  else
    return FALSE;
}

static void
btk_widget_real_move_focus (BtkWidget         *widget,
                            BtkDirectionType   direction)
{
  BtkWidget *toplevel = btk_widget_get_toplevel (widget);

  if (BTK_IS_WINDOW (toplevel) &&
      BTK_WINDOW_GET_CLASS (toplevel)->move_focus)
    {
      BTK_WINDOW_GET_CLASS (toplevel)->move_focus (BTK_WINDOW (toplevel),
                                                   direction);
    }
}

static gboolean
btk_widget_real_keynav_failed (BtkWidget        *widget,
                               BtkDirectionType  direction)
{
  gboolean cursor_only;

  switch (direction)
    {
    case BTK_DIR_TAB_FORWARD:
    case BTK_DIR_TAB_BACKWARD:
      return FALSE;

    case BTK_DIR_UP:
    case BTK_DIR_DOWN:
    case BTK_DIR_LEFT:
    case BTK_DIR_RIGHT:
      g_object_get (btk_widget_get_settings (widget),
                    "btk-keynav-cursor-only", &cursor_only,
                    NULL);
      if (cursor_only)
        return FALSE;
      break;
    }

  btk_widget_error_bell (widget);

  return TRUE;
}

/**
 * btk_widget_set_can_focus:
 * @widget: a #BtkWidget
 * @can_focus: whether or not @widget can own the input focus.
 *
 * Specifies whether @widget can own the input focus. See
 * btk_widget_grab_focus() for actually setting the input focus on a
 * widget.
 *
 * Since: 2.18
 **/
void
btk_widget_set_can_focus (BtkWidget *widget,
                          gboolean   can_focus)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (can_focus != btk_widget_get_can_focus (widget))
    {
      if (can_focus)
        BTK_OBJECT_FLAGS (widget) |= BTK_CAN_FOCUS;
      else
        BTK_OBJECT_FLAGS (widget) &= ~(BTK_CAN_FOCUS);

      btk_widget_queue_resize (widget);
      g_object_notify (G_OBJECT (widget), "can-focus");
    }
}

/**
 * btk_widget_get_can_focus:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget can own the input focus. See
 * btk_widget_set_can_focus().
 *
 * Return value: %TRUE if @widget can own the input focus, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_can_focus (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_CAN_FOCUS) != 0;
}

/**
 * btk_widget_has_focus:
 * @widget: a #BtkWidget
 *
 * Determines if the widget has the global input focus. See
 * btk_widget_is_focus() for the difference between having the global
 * input focus, and only having the focus within a toplevel.
 *
 * Return value: %TRUE if the widget has the global input focus.
 *
 * Since: 2.18
 **/
gboolean
btk_widget_has_focus (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_HAS_FOCUS) != 0;
}

/**
 * btk_widget_is_focus:
 * @widget: a #BtkWidget
 * 
 * Determines if the widget is the focus widget within its
 * toplevel. (This does not mean that the %HAS_FOCUS flag is
 * necessarily set; %HAS_FOCUS will only be set if the
 * toplevel widget additionally has the global input focus.)
 * 
 * Return value: %TRUE if the widget is the focus widget.
 **/
gboolean
btk_widget_is_focus (BtkWidget *widget)
{
  BtkWidget *toplevel;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  toplevel = btk_widget_get_toplevel (widget);
  
  if (BTK_IS_WINDOW (toplevel))
    return widget == BTK_WINDOW (toplevel)->focus_widget;
  else
    return FALSE;
}

/**
 * btk_widget_set_can_default:
 * @widget: a #BtkWidget
 * @can_default: whether or not @widget can be a default widget.
 *
 * Specifies whether @widget can be a default widget. See
 * btk_widget_grab_default() for details about the meaning of
 * "default".
 *
 * Since: 2.18
 **/
void
btk_widget_set_can_default (BtkWidget *widget,
                            gboolean   can_default)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (can_default != btk_widget_get_can_default (widget))
    {
      if (can_default)
        BTK_OBJECT_FLAGS (widget) |= BTK_CAN_DEFAULT;
      else
        BTK_OBJECT_FLAGS (widget) &= ~(BTK_CAN_DEFAULT);

      btk_widget_queue_resize (widget);
      g_object_notify (G_OBJECT (widget), "can-default");
    }
}

/**
 * btk_widget_get_can_default:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget can be a default widget. See
 * btk_widget_set_can_default().
 *
 * Return value: %TRUE if @widget can be a default widget, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_can_default (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_CAN_DEFAULT) != 0;
}

/**
 * btk_widget_has_default:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget is the current default widget within its
 * toplevel. See btk_widget_set_can_default().
 *
 * Return value: %TRUE if @widget is the current default widget within
 *     its toplevel, %FALSE otherwise
 *
 * Since: 2.18
 */
gboolean
btk_widget_has_default (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_HAS_DEFAULT) != 0;
}

void
_btk_widget_set_has_default (BtkWidget *widget,
                             gboolean   has_default)
{
  if (has_default)
    BTK_OBJECT_FLAGS (widget) |= BTK_HAS_DEFAULT;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_HAS_DEFAULT);
}

/**
 * btk_widget_grab_default:
 * @widget: a #BtkWidget
 *
 * Causes @widget to become the default widget. @widget must have the
 * %BTK_CAN_DEFAULT flag set; typically you have to set this flag
 * yourself by calling <literal>btk_widget_set_can_default (@widget,
 * %TRUE)</literal>. The default widget is activated when 
 * the user presses Enter in a window. Default widgets must be 
 * activatable, that is, btk_widget_activate() should affect them.
 **/
void
btk_widget_grab_default (BtkWidget *widget)
{
  BtkWidget *window;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (btk_widget_get_can_default (widget));
  
  window = btk_widget_get_toplevel (widget);
  
  if (window && btk_widget_is_toplevel (window))
    btk_window_set_default (BTK_WINDOW (window), widget);
  else
    g_warning (B_STRLOC ": widget not within a BtkWindow");
}

/**
 * btk_widget_set_receives_default:
 * @widget: a #BtkWidget
 * @receives_default: whether or not @widget can be a default widget.
 *
 * Specifies whether @widget will be treated as the default widget
 * within its toplevel when it has the focus, even if another widget
 * is the default.
 *
 * See btk_widget_grab_default() for details about the meaning of
 * "default".
 *
 * Since: 2.18
 **/
void
btk_widget_set_receives_default (BtkWidget *widget,
                                 gboolean   receives_default)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (receives_default != btk_widget_get_receives_default (widget))
    {
      if (receives_default)
        BTK_OBJECT_FLAGS (widget) |= BTK_RECEIVES_DEFAULT;
      else
        BTK_OBJECT_FLAGS (widget) &= ~(BTK_RECEIVES_DEFAULT);

      g_object_notify (G_OBJECT (widget), "receives-default");
    }
}

/**
 * btk_widget_get_receives_default:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget is alyways treated as default widget
 * withing its toplevel when it has the focus, even if another widget
 * is the default.
 *
 * See btk_widget_set_receives_default().
 *
 * Return value: %TRUE if @widget acts as default widget when focussed,
 *               %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_receives_default (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_RECEIVES_DEFAULT) != 0;
}

/**
 * btk_widget_has_grab:
 * @widget: a #BtkWidget
 *
 * Determines whether the widget is currently grabbing events, so it
 * is the only widget receiving input events (keyboard and mouse).
 *
 * See also btk_grab_add().
 *
 * Return value: %TRUE if the widget is in the grab_widgets stack
 *
 * Since: 2.18
 **/
gboolean
btk_widget_has_grab (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_HAS_GRAB) != 0;
}

void
_btk_widget_set_has_grab (BtkWidget *widget,
                          gboolean   has_grab)
{
  if (has_grab)
    BTK_OBJECT_FLAGS (widget) |= BTK_HAS_GRAB;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_HAS_GRAB);
}

/**
 * btk_widget_set_name:
 * @widget: a #BtkWidget
 * @name: name for the widget
 *
 * Widgets can be named, which allows you to refer to them from a
 * btkrc file. You can apply a style to widgets with a particular name
 * in the btkrc file. See the documentation for btkrc files (on the
 * same page as the docs for #BtkRcStyle).
 * 
 * Note that widget names are separated by periods in paths (see 
 * btk_widget_path()), so names with embedded periods may cause confusion.
 **/
void
btk_widget_set_name (BtkWidget	 *widget,
		     const gchar *name)
{
  gchar *new_name;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));

  new_name = g_strdup (name);
  g_free (widget->name);
  widget->name = new_name;

  if (btk_widget_has_rc_style (widget))
    btk_widget_reset_rc_style (widget);

  g_object_notify (G_OBJECT (widget), "name");
}

/**
 * btk_widget_get_name:
 * @widget: a #BtkWidget
 * 
 * Retrieves the name of a widget. See btk_widget_set_name() for the
 * significance of widget names.
 * 
 * Return value: name of the widget. This string is owned by BTK+ and
 * should not be modified or freed
 **/
const gchar*
btk_widget_get_name (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  if (widget->name)
    return widget->name;
  return G_OBJECT_TYPE_NAME (widget);
}

/**
 * btk_widget_set_state:
 * @widget: a #BtkWidget
 * @state: new state for @widget
 *
 * This function is for use in widget implementations. Sets the state
 * of a widget (insensitive, prelighted, etc.) Usually you should set
 * the state using wrapper functions such as btk_widget_set_sensitive().
 **/
void
btk_widget_set_state (BtkWidget           *widget,
		      BtkStateType         state)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (state == btk_widget_get_state (widget))
    return;

  if (state == BTK_STATE_INSENSITIVE)
    btk_widget_set_sensitive (widget, FALSE);
  else
    {
      BtkStateData data;

      data.state = state;
      data.state_restoration = FALSE;
      data.use_forall = FALSE;
      if (widget->parent)
	data.parent_sensitive = (btk_widget_is_sensitive (widget->parent) != FALSE);
      else
	data.parent_sensitive = TRUE;

      btk_widget_propagate_state (widget, &data);
  
      if (btk_widget_is_drawable (widget))
	btk_widget_queue_draw (widget);
    }
}

/**
 * btk_widget_get_state:
 * @widget: a #BtkWidget
 *
 * Returns the widget's state. See btk_widget_set_state().
 *
 * Returns: the state of @widget.
 *
 * Since: 2.18
 */
BtkStateType
btk_widget_get_state (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), BTK_STATE_NORMAL);

  return widget->state;
}

/**
 * btk_widget_set_visible:
 * @widget: a #BtkWidget
 * @visible: whether the widget should be shown or not
 *
 * Sets the visibility state of @widget. Note that setting this to
 * %TRUE doesn't mean the widget is actually viewable, see
 * btk_widget_get_visible().
 *
 * This function simply calls btk_widget_show() or btk_widget_hide()
 * but is nicer to use when the visibility of the widget depends on
 * some condition.
 *
 * Since: 2.18
 **/
void
btk_widget_set_visible (BtkWidget *widget,
                        gboolean   visible)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (visible != btk_widget_get_visible (widget))
    {
      if (visible)
        btk_widget_show (widget);
      else
        btk_widget_hide (widget);
    }
}

/**
 * btk_widget_get_visible:
 * @widget: a #BtkWidget
 *
 * Determines whether the widget is visible. Note that this doesn't
 * take into account whether the widget's parent is also visible
 * or the widget is obscured in any way.
 *
 * See btk_widget_set_visible().
 *
 * Return value: %TRUE if the widget is visible
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_visible (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_VISIBLE) != 0;
}

/**
 * btk_widget_set_has_window:
 * @widget: a #BtkWidget
 * @has_window: whether or not @widget has a window.
 *
 * Specifies whether @widget has a #BdkWindow of its own. Note that
 * all realized widgets have a non-%NULL "window" pointer
 * (btk_widget_get_window() never returns a %NULL window when a widget
 * is realized), but for many of them it's actually the #BdkWindow of
 * one of its parent widgets. Widgets that do not create a %window for
 * themselves in BtkWidget::realize() must announce this by
 * calling this function with @has_window = %FALSE.
 *
 * This function should only be called by widget implementations,
 * and they should call it in their init() function.
 *
 * Since: 2.18
 **/
void
btk_widget_set_has_window (BtkWidget *widget,
                           gboolean   has_window)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (has_window)
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_NO_WINDOW);
  else
    BTK_OBJECT_FLAGS (widget) |= BTK_NO_WINDOW;
}

/**
 * btk_widget_get_has_window:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget has a #BdkWindow of its own. See
 * btk_widget_set_has_window().
 *
 * Return value: %TRUE if @widget has a window, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_has_window (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return !((BTK_OBJECT_FLAGS (widget) & BTK_NO_WINDOW) != 0);
}

/**
 * btk_widget_is_toplevel:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget is a toplevel widget. Currently only
 * #BtkWindow and #BtkInvisible are toplevel widgets. Toplevel
 * widgets have no parent widget.
 *
 * Return value: %TRUE if @widget is a toplevel, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
btk_widget_is_toplevel (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_TOPLEVEL) != 0;
}

void
_btk_widget_set_is_toplevel (BtkWidget *widget,
                             gboolean   is_toplevel)
{
  if (is_toplevel)
    BTK_OBJECT_FLAGS (widget) |= BTK_TOPLEVEL;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_TOPLEVEL);
}

/**
 * btk_widget_is_drawable:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget can be drawn to. A widget can be drawn
 * to if it is mapped and visible.
 *
 * Return value: %TRUE if @widget is drawable, %FALSE otherwise
 *
 * Since: 2.18
 **/
gboolean
btk_widget_is_drawable (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (btk_widget_get_visible (widget) &&
          btk_widget_get_mapped (widget));
}

/**
 * btk_widget_get_realized:
 * @widget: a #BtkWidget
 *
 * Determines whether @widget is realized.
 *
 * Return value: %TRUE if @widget is realized, %FALSE otherwise
 *
 * Since: 2.20
 **/
gboolean
btk_widget_get_realized (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_REALIZED) != 0;
}

/**
 * btk_widget_set_realized:
 * @widget: a #BtkWidget
 * @realized: %TRUE to mark the widget as realized
 *
 * Marks the widget as being realized.
 *
 * This function should only ever be called in a derived widget's
 * "realize" or "unrealize" implementation.
 *
 * Since: 2.20
 */
void
btk_widget_set_realized (BtkWidget *widget,
                         gboolean   realized)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (realized)
    BTK_OBJECT_FLAGS (widget) |= BTK_REALIZED;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_REALIZED);
}

/**
 * btk_widget_get_mapped:
 * @widget: a #BtkWidget
 *
 * Whether the widget is mapped.
 *
 * Return value: %TRUE if the widget is mapped, %FALSE otherwise.
 *
 * Since: 2.20
 */
gboolean
btk_widget_get_mapped (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_MAPPED) != 0;
}

/**
 * btk_widget_set_mapped:
 * @widget: a #BtkWidget
 * @mapped: %TRUE to mark the widget as mapped
 *
 * Marks the widget as being realized.
 *
 * This function should only ever be called in a derived widget's
 * "map" or "unmap" implementation.
 *
 * Since: 2.20
 */
void
btk_widget_set_mapped (BtkWidget *widget,
                       gboolean   mapped)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (mapped)
    BTK_OBJECT_FLAGS (widget) |= BTK_MAPPED;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_MAPPED);
}

/**
 * btk_widget_set_app_paintable:
 * @widget: a #BtkWidget
 * @app_paintable: %TRUE if the application will paint on the widget
 *
 * Sets whether the application intends to draw on the widget in
 * an #BtkWidget::expose-event handler. 
 *
 * This is a hint to the widget and does not affect the behavior of 
 * the BTK+ core; many widgets ignore this flag entirely. For widgets 
 * that do pay attention to the flag, such as #BtkEventBox and #BtkWindow, 
 * the effect is to suppress default themed drawing of the widget's 
 * background. (Children of the widget will still be drawn.) The application 
 * is then entirely responsible for drawing the widget background.
 *
 * Note that the background is still drawn when the widget is mapped.
 * If this is not suitable (e.g. because you want to make a transparent
 * window using an RGBA visual), you can work around this by doing:
 * |[
 *  btk_widget_realize (window);
 *  bdk_window_set_back_pixmap (window->window, NULL, FALSE);
 *  btk_widget_show (window);
 * ]|
 **/
void
btk_widget_set_app_paintable (BtkWidget *widget,
			      gboolean   app_paintable)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  app_paintable = (app_paintable != FALSE);

  if (btk_widget_get_app_paintable (widget) != app_paintable)
    {
      if (app_paintable)
        BTK_OBJECT_FLAGS (widget) |= BTK_APP_PAINTABLE;
      else
        BTK_OBJECT_FLAGS (widget) &= ~(BTK_APP_PAINTABLE);

      if (btk_widget_is_drawable (widget))
	btk_widget_queue_draw (widget);

      g_object_notify (G_OBJECT (widget), "app-paintable");
    }
}

/**
 * btk_widget_get_app_paintable:
 * @widget: a #BtkWidget
 *
 * Determines whether the application intends to draw on the widget in
 * an #BtkWidget::expose-event handler.
 *
 * See btk_widget_set_app_paintable()
 *
 * Return value: %TRUE if the widget is app paintable
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_app_paintable (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_APP_PAINTABLE) != 0;
}

/**
 * btk_widget_set_double_buffered:
 * @widget: a #BtkWidget
 * @double_buffered: %TRUE to double-buffer a widget
 *
 * Widgets are double buffered by default; you can use this function
 * to turn off the buffering. "Double buffered" simply means that
 * bdk_window_begin_paint_rebunnyion() and bdk_window_end_paint() are called
 * automatically around expose events sent to the
 * widget. bdk_window_begin_paint() diverts all drawing to a widget's
 * window to an offscreen buffer, and bdk_window_end_paint() draws the
 * buffer to the screen. The result is that users see the window
 * update in one smooth step, and don't see individual graphics
 * primitives being rendered.
 *
 * In very simple terms, double buffered widgets don't flicker,
 * so you would only use this function to turn off double buffering
 * if you had special needs and really knew what you were doing.
 * 
 * Note: if you turn off double-buffering, you have to handle
 * expose events, since even the clearing to the background color or 
 * pixmap will not happen automatically (as it is done in 
 * bdk_window_begin_paint()).
 **/
void
btk_widget_set_double_buffered (BtkWidget *widget,
				gboolean   double_buffered)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  double_buffered = (double_buffered != FALSE);

  if (double_buffered != btk_widget_get_double_buffered (widget))
    {
      if (double_buffered)
        BTK_OBJECT_FLAGS (widget) |= BTK_DOUBLE_BUFFERED;
      else
        BTK_OBJECT_FLAGS (widget) &= ~(BTK_DOUBLE_BUFFERED);

      g_object_notify (G_OBJECT (widget), "double-buffered");
    }
}

/**
 * btk_widget_get_double_buffered:
 * @widget: a #BtkWidget
 *
 * Determines whether the widget is double buffered.
 *
 * See btk_widget_set_double_buffered()
 *
 * Return value: %TRUE if the widget is double buffered
 *
 * Since: 2.18
 **/
gboolean
btk_widget_get_double_buffered (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_DOUBLE_BUFFERED) != 0;
}

/**
 * btk_widget_set_redraw_on_allocate:
 * @widget: a #BtkWidget
 * @redraw_on_allocate: if %TRUE, the entire widget will be redrawn
 *   when it is allocated to a new size. Otherwise, only the
 *   new portion of the widget will be redrawn.
 *
 * Sets whether the entire widget is queued for drawing when its size 
 * allocation changes. By default, this setting is %TRUE and
 * the entire widget is redrawn on every size change. If your widget
 * leaves the upper left unchanged when made bigger, turning this
 * setting off will improve performance.

 * Note that for %NO_WINDOW widgets setting this flag to %FALSE turns
 * off all allocation on resizing: the widget will not even redraw if
 * its position changes; this is to allow containers that don't draw
 * anything to avoid excess invalidations. If you set this flag on a
 * %NO_WINDOW widget that <emphasis>does</emphasis> draw on @widget->window, 
 * you are responsible for invalidating both the old and new allocation 
 * of the widget when the widget is moved and responsible for invalidating
 * rebunnyions newly when the widget increases size.
 **/
void
btk_widget_set_redraw_on_allocate (BtkWidget *widget,
				   gboolean   redraw_on_allocate)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (redraw_on_allocate)
    BTK_PRIVATE_SET_FLAG (widget, BTK_REDRAW_ON_ALLOC);
  else
    BTK_PRIVATE_UNSET_FLAG (widget, BTK_REDRAW_ON_ALLOC);
}

/**
 * btk_widget_set_sensitive:
 * @widget: a #BtkWidget
 * @sensitive: %TRUE to make the widget sensitive
 *
 * Sets the sensitivity of a widget. A widget is sensitive if the user
 * can interact with it. Insensitive widgets are "grayed out" and the
 * user can't interact with them. Insensitive widgets are known as
 * "inactive", "disabled", or "ghosted" in some other toolkits.
 **/
void
btk_widget_set_sensitive (BtkWidget *widget,
			  gboolean   sensitive)
{
  BtkStateData data;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  sensitive = (sensitive != FALSE);

  if (sensitive == (btk_widget_get_sensitive (widget) != FALSE))
    return;

  if (sensitive)
    {
      BTK_OBJECT_FLAGS (widget) |= BTK_SENSITIVE;
      data.state = widget->saved_state;
    }
  else
    {
      BTK_OBJECT_FLAGS (widget) &= ~(BTK_SENSITIVE);
      data.state = btk_widget_get_state (widget);
    }
  data.state_restoration = TRUE;
  data.use_forall = TRUE;

  if (widget->parent)
    data.parent_sensitive = (btk_widget_is_sensitive (widget->parent) != FALSE);
  else
    data.parent_sensitive = TRUE;

  btk_widget_propagate_state (widget, &data);
  if (btk_widget_is_drawable (widget))
    btk_widget_queue_draw (widget);

  g_object_notify (G_OBJECT (widget), "sensitive");
}

/**
 * btk_widget_get_sensitive:
 * @widget: a #BtkWidget
 *
 * Returns the widget's sensitivity (in the sense of returning
 * the value that has been set using btk_widget_set_sensitive()).
 *
 * The effective sensitivity of a widget is however determined by both its
 * own and its parent widget's sensitivity. See btk_widget_is_sensitive().
 *
 * Returns: %TRUE if the widget is sensitive
 *
 * Since: 2.18
 */
gboolean
btk_widget_get_sensitive (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_SENSITIVE) != 0;
}

/**
 * btk_widget_is_sensitive:
 * @widget: a #BtkWidget
 *
 * Returns the widget's effective sensitivity, which means
 * it is sensitive itself and also its parent widget is sensntive
 *
 * Returns: %TRUE if the widget is effectively sensitive
 *
 * Since: 2.18
 */
gboolean
btk_widget_is_sensitive (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (btk_widget_get_sensitive (widget) &&
          (BTK_OBJECT_FLAGS (widget) & BTK_PARENT_SENSITIVE) != 0);
}

/**
 * btk_widget_set_parent:
 * @widget: a #BtkWidget
 * @parent: parent container
 *
 * This function is useful only when implementing subclasses of 
 * #BtkContainer.
 * Sets the container as the parent of @widget, and takes care of
 * some details such as updating the state and style of the child
 * to reflect its new location. The opposite function is
 * btk_widget_unparent().
 **/
void
btk_widget_set_parent (BtkWidget *widget,
		       BtkWidget *parent)
{
  BtkStateData data;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_IS_WIDGET (parent));
  g_return_if_fail (widget != parent);
  if (widget->parent != NULL)
    {
      g_warning ("Can't set a parent on widget which has a parent\n");
      return;
    }
  if (btk_widget_is_toplevel (widget))
    {
      g_warning ("Can't set a parent on a toplevel widget\n");
      return;
    }

  /* keep this function in sync with btk_menu_attach_to_widget()
   */

  g_object_ref_sink (widget);
  widget->parent = parent;

  if (btk_widget_get_state (parent) != BTK_STATE_NORMAL)
    data.state = btk_widget_get_state (parent);
  else
    data.state = btk_widget_get_state (widget);
  data.state_restoration = FALSE;
  data.parent_sensitive = (btk_widget_is_sensitive (parent) != FALSE);
  data.use_forall = btk_widget_is_sensitive (parent) != btk_widget_is_sensitive (widget);

  btk_widget_propagate_state (widget, &data);
  
  btk_widget_reset_rc_styles (widget);

  g_signal_emit (widget, widget_signals[PARENT_SET], 0, NULL);
  if (BTK_WIDGET_ANCHORED (widget->parent))
    _btk_widget_propagate_hierarchy_changed (widget, NULL);
  g_object_notify (G_OBJECT (widget), "parent");

  /* Enforce realized/mapped invariants
   */
  if (btk_widget_get_realized (widget->parent))
    btk_widget_realize (widget);

  if (btk_widget_get_visible (widget->parent) &&
      btk_widget_get_visible (widget))
    {
      if (BTK_WIDGET_CHILD_VISIBLE (widget) &&
	  btk_widget_get_mapped (widget->parent))
	btk_widget_map (widget);

      btk_widget_queue_resize (widget);
    }
}

/**
 * btk_widget_get_parent:
 * @widget: a #BtkWidget
 *
 * Returns the parent container of @widget.
 *
 * Return value: (transfer none): the parent container of @widget, or %NULL
 **/
BtkWidget *
btk_widget_get_parent (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  return widget->parent;
}

/*****************************************
 * Widget styles
 * see docs/styles.txt
 *****************************************/

/**
 * btk_widget_style_attach:
 * @widget: a #BtkWidget
 *
 * This function attaches the widget's #BtkStyle to the widget's
 * #BdkWindow. It is a replacement for
 *
 * <programlisting>
 * widget->style = btk_style_attach (widget->style, widget->window);
 * </programlisting>
 *
 * and should only ever be called in a derived widget's "realize"
 * implementation which does not chain up to its parent class'
 * "realize" implementation, because one of the parent classes
 * (finally #BtkWidget) would attach the style itself.
 *
 * Since: 2.20
 **/
void
btk_widget_style_attach (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (btk_widget_get_realized (widget));

  widget->style = btk_style_attach (widget->style, widget->window);
}

/**
 * btk_widget_has_rc_style:
 * @widget: a #BtkWidget
 *
 * Determines if the widget style has been looked up through the rc mechanism.
 *
 * Returns: %TRUE if the widget has been looked up through the rc
 *   mechanism, %FALSE otherwise.
 *
 * Since: 2.20
 **/
gboolean
btk_widget_has_rc_style (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (BTK_OBJECT_FLAGS (widget) & BTK_RC_STYLE) != 0;
}

/**
 * btk_widget_set_style:
 * @widget: a #BtkWidget
 * @style: (allow-none): a #BtkStyle, or %NULL to remove the effect of a previous
 *         btk_widget_set_style() and go back to the default style
 *
 * Sets the #BtkStyle for a widget (@widget->style). You probably don't
 * want to use this function; it interacts badly with themes, because
 * themes work by replacing the #BtkStyle. Instead, use
 * btk_widget_modify_style().
 **/
void
btk_widget_set_style (BtkWidget *widget,
		      BtkStyle	*style)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (style)
    {
      gboolean initial_emission;

      initial_emission = !btk_widget_has_rc_style (widget) && !BTK_WIDGET_USER_STYLE (widget);
      
      BTK_OBJECT_FLAGS (widget) &= ~(BTK_RC_STYLE);
      BTK_PRIVATE_SET_FLAG (widget, BTK_USER_STYLE);
      
      btk_widget_set_style_internal (widget, style, initial_emission);
    }
  else
    {
      if (BTK_WIDGET_USER_STYLE (widget))
	btk_widget_reset_rc_style (widget);
    }
}

/**
 * btk_widget_ensure_style:
 * @widget: a #BtkWidget
 *
 * Ensures that @widget has a style (@widget->style). Not a very useful
 * function; most of the time, if you want the style, the widget is
 * realized, and realized widgets are guaranteed to have a style
 * already.
 **/
void
btk_widget_ensure_style (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (!BTK_WIDGET_USER_STYLE (widget) &&
      !btk_widget_has_rc_style (widget))
    btk_widget_reset_rc_style (widget);
}

/* Look up the RC style for this widget, unsetting any user style that
 * may be in effect currently
 **/
static void
btk_widget_reset_rc_style (BtkWidget *widget)
{
  BtkStyle *new_style = NULL;
  gboolean initial_emission;
  
  initial_emission = !btk_widget_has_rc_style (widget) && !BTK_WIDGET_USER_STYLE (widget);

  BTK_PRIVATE_UNSET_FLAG (widget, BTK_USER_STYLE);
  BTK_OBJECT_FLAGS (widget) |= BTK_RC_STYLE;
  
  if (btk_widget_has_screen (widget))
    new_style = btk_rc_get_style (widget);
  if (!new_style)
    new_style = btk_widget_get_default_style ();

  if (initial_emission || new_style != widget->style)
    btk_widget_set_style_internal (widget, new_style, initial_emission);
}

/**
 * btk_widget_get_style:
 * @widget: a #BtkWidget
 * 
 * Simply an accessor function that returns @widget->style.
 *
 * Return value: (transfer none): the widget's #BtkStyle
 **/
BtkStyle*
btk_widget_get_style (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  return widget->style;
}

/**
 * btk_widget_modify_style:
 * @widget: a #BtkWidget
 * @style: the #BtkRcStyle holding the style modifications
 * 
 * Modifies style values on the widget. Modifications made using this
 * technique take precedence over style values set via an RC file,
 * however, they will be overriden if a style is explicitely set on
 * the widget using btk_widget_set_style(). The #BtkRcStyle structure
 * is designed so each field can either be set or unset, so it is
 * possible, using this function, to modify some style values and
 * leave the others unchanged.
 *
 * Note that modifications made with this function are not cumulative
 * with previous calls to btk_widget_modify_style() or with such
 * functions as btk_widget_modify_fg(). If you wish to retain
 * previous values, you must first call btk_widget_get_modifier_style(),
 * make your modifications to the returned style, then call
 * btk_widget_modify_style() with that style. On the other hand,
 * if you first call btk_widget_modify_style(), subsequent calls
 * to such functions btk_widget_modify_fg() will have a cumulative
 * effect with the initial modifications.
 **/
void       
btk_widget_modify_style (BtkWidget      *widget,
			 BtkRcStyle     *style)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_IS_RC_STYLE (style));
  
  g_object_set_qdata_full (G_OBJECT (widget),
			   quark_rc_style,
			   btk_rc_style_copy (style),
			   (GDestroyNotify) g_object_unref);

  /* note that "style" may be invalid here if it was the old
   * modifier style and the only reference was our own.
   */
  
  if (btk_widget_has_rc_style (widget))
    btk_widget_reset_rc_style (widget);
}

/**
 * btk_widget_get_modifier_style:
 * @widget: a #BtkWidget
 * 
 * Returns the current modifier style for the widget. (As set by
 * btk_widget_modify_style().) If no style has previously set, a new
 * #BtkRcStyle will be created with all values unset, and set as the
 * modifier style for the widget. If you make changes to this rc
 * style, you must call btk_widget_modify_style(), passing in the
 * returned rc style, to make sure that your changes take effect.
 *
 * Caution: passing the style back to btk_widget_modify_style() will
 * normally end up destroying it, because btk_widget_modify_style() copies
 * the passed-in style and sets the copy as the new modifier style,
 * thus dropping any reference to the old modifier style. Add a reference
 * to the modifier style if you want to keep it alive.
 *
 * Return value: (transfer none): the modifier style for the widget. This rc style is
 *   owned by the widget. If you want to keep a pointer to value this
 *   around, you must add a refcount using g_object_ref().
 **/
BtkRcStyle *
btk_widget_get_modifier_style (BtkWidget      *widget)
{
  BtkRcStyle *rc_style;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  rc_style = g_object_get_qdata (G_OBJECT (widget), quark_rc_style);

  if (!rc_style)
    {
      rc_style = btk_rc_style_new ();
      g_object_set_qdata_full (G_OBJECT (widget),
			       quark_rc_style,
			       rc_style,
			       (GDestroyNotify) g_object_unref);
    }

  return rc_style;
}

static void
btk_widget_modify_color_component (BtkWidget      *widget,
				   BtkRcFlags      component,
				   BtkStateType    state,
				   const BdkColor *color)
{
  BtkRcStyle *rc_style = btk_widget_get_modifier_style (widget);  

  if (color)
    {
      switch (component)
	{
	case BTK_RC_FG:
	  rc_style->fg[state] = *color;
	  break;
	case BTK_RC_BG:
	  rc_style->bg[state] = *color;
	  break;
	case BTK_RC_TEXT:
	  rc_style->text[state] = *color;
	  break;
	case BTK_RC_BASE:
	  rc_style->base[state] = *color;
	  break;
	default:
	  g_assert_not_reached();
	}
      
      rc_style->color_flags[state] |= component;
    }
  else
    rc_style->color_flags[state] &= ~component;

  btk_widget_modify_style (widget, rc_style);
}

/**
 * btk_widget_modify_fg:
 * @widget: a #BtkWidget
 * @state: the state for which to set the foreground color
 * @color: (allow-none): the color to assign (does not need to be allocated),
 *         or %NULL to undo the effect of previous calls to
 *         of btk_widget_modify_fg().
 *
 * Sets the foreground color for a widget in a particular state.
 * All other style values are left untouched. See also
 * btk_widget_modify_style().
 **/
void
btk_widget_modify_fg (BtkWidget      *widget,
		      BtkStateType    state,
		      const BdkColor *color)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (state >= BTK_STATE_NORMAL && state <= BTK_STATE_INSENSITIVE);

  btk_widget_modify_color_component (widget, BTK_RC_FG, state, color);
}

/**
 * btk_widget_modify_bg:
 * @widget: a #BtkWidget
 * @state: the state for which to set the background color
 * @color: (allow-none): the color to assign (does not need to be allocated),
 *         or %NULL to undo the effect of previous calls to
 *         of btk_widget_modify_bg().
 *
 * Sets the background color for a widget in a particular state.
 * All other style values are left untouched. See also
 * btk_widget_modify_style(). 
 *
 * Note that "no window" widgets (which have the %BTK_NO_WINDOW flag set)
 * draw on their parent container's window and thus may not draw any 
 * background themselves. This is the case for e.g. #BtkLabel. To modify 
 * the background of such widgets, you have to set the background color 
 * on their parent; if you want to set the background of a rectangular 
 * area around a label, try placing the label in a #BtkEventBox widget 
 * and setting the background color on that.
 **/
void
btk_widget_modify_bg (BtkWidget      *widget,
		      BtkStateType    state,
		      const BdkColor *color)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (state >= BTK_STATE_NORMAL && state <= BTK_STATE_INSENSITIVE);

  btk_widget_modify_color_component (widget, BTK_RC_BG, state, color);
}

/**
 * btk_widget_modify_text:
 * @widget: a #BtkWidget
 * @state: the state for which to set the text color
 * @color: (allow-none): the color to assign (does not need to be allocated),
 *         or %NULL to undo the effect of previous calls to
 *         of btk_widget_modify_text().
 *
 * Sets the text color for a widget in a particular state.  All other
 * style values are left untouched. The text color is the foreground
 * color used along with the base color (see btk_widget_modify_base())
 * for widgets such as #BtkEntry and #BtkTextView. See also
 * btk_widget_modify_style().
 **/
void
btk_widget_modify_text (BtkWidget      *widget,
			BtkStateType    state,
			const BdkColor *color)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (state >= BTK_STATE_NORMAL && state <= BTK_STATE_INSENSITIVE);

  btk_widget_modify_color_component (widget, BTK_RC_TEXT, state, color);
}

/**
 * btk_widget_modify_base:
 * @widget: a #BtkWidget
 * @state: the state for which to set the base color
 * @color: (allow-none): the color to assign (does not need to be allocated),
 *         or %NULL to undo the effect of previous calls to
 *         of btk_widget_modify_base().
 *
 * Sets the base color for a widget in a particular state.
 * All other style values are left untouched. The base color
 * is the background color used along with the text color
 * (see btk_widget_modify_text()) for widgets such as #BtkEntry
 * and #BtkTextView. See also btk_widget_modify_style().
 *
 * Note that "no window" widgets (which have the %BTK_NO_WINDOW flag set)
 * draw on their parent container's window and thus may not draw any 
 * background themselves. This is the case for e.g. #BtkLabel. To modify 
 * the background of such widgets, you have to set the base color on their 
 * parent; if you want to set the background of a rectangular area around 
 * a label, try placing the label in a #BtkEventBox widget and setting 
 * the base color on that.
 **/
void
btk_widget_modify_base (BtkWidget      *widget,
			BtkStateType    state,
			const BdkColor *color)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (state >= BTK_STATE_NORMAL && state <= BTK_STATE_INSENSITIVE);

  btk_widget_modify_color_component (widget, BTK_RC_BASE, state, color);
}

static void
modify_color_property (BtkWidget      *widget,
		       BtkRcStyle     *rc_style,
		       const char     *name,
		       const BdkColor *color)
{
  GQuark type_name = g_type_qname (G_OBJECT_TYPE (widget));
  GQuark property_name = g_quark_from_string (name);

  if (color)
    {
      BtkRcProperty rc_property = {0};
      char *color_name;

      rc_property.type_name = type_name;
      rc_property.property_name = property_name;
      rc_property.origin = NULL;

      color_name = bdk_color_to_string (color);
      g_value_init (&rc_property.value, G_TYPE_STRING);
      g_value_take_string (&rc_property.value, color_name);

      _btk_rc_style_set_rc_property (rc_style, &rc_property);

      g_value_unset (&rc_property.value);
    }
  else
    _btk_rc_style_unset_rc_property (rc_style, type_name, property_name);
}

/**
 * btk_widget_modify_cursor:
 * @widget: a #BtkWidget
 * @primary: the color to use for primary cursor (does not need to be
 *           allocated), or %NULL to undo the effect of previous calls to
 *           of btk_widget_modify_cursor().
 * @secondary: the color to use for secondary cursor (does not need to be
 *             allocated), or %NULL to undo the effect of previous calls to
 *             of btk_widget_modify_cursor().
 *
 * Sets the cursor color to use in a widget, overriding the
 * #BtkWidget:cursor-color and #BtkWidget:secondary-cursor-color
 * style properties. All other style values are left untouched. 
 * See also btk_widget_modify_style().
 *
 * Since: 2.12
 **/
void
btk_widget_modify_cursor (BtkWidget      *widget,
			  const BdkColor *primary,
			  const BdkColor *secondary)
{
  BtkRcStyle *rc_style;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  rc_style = btk_widget_get_modifier_style (widget);

  modify_color_property (widget, rc_style, "cursor-color", primary);
  modify_color_property (widget, rc_style, "secondary-cursor-color", secondary);

  btk_widget_modify_style (widget, rc_style);
}

/**
 * btk_widget_modify_font:
 * @widget: a #BtkWidget
 * @font_desc: (allow-none): the font description to use, or %NULL to undo
 *   the effect of previous calls to btk_widget_modify_font().
 *
 * Sets the font to use for a widget.  All other style values are left
 * untouched. See also btk_widget_modify_style().
 **/
void
btk_widget_modify_font (BtkWidget            *widget,
			BangoFontDescription *font_desc)
{
  BtkRcStyle *rc_style;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  rc_style = btk_widget_get_modifier_style (widget);  

  if (rc_style->font_desc)
    bango_font_description_free (rc_style->font_desc);

  if (font_desc)
    rc_style->font_desc = bango_font_description_copy (font_desc);
  else
    rc_style->font_desc = NULL;
  
  btk_widget_modify_style (widget, rc_style);
}

static void
btk_widget_real_direction_changed (BtkWidget        *widget,
                                   BtkTextDirection  previous_direction)
{
  btk_widget_queue_resize (widget);
}

static void
btk_widget_real_style_set (BtkWidget *widget,
                           BtkStyle  *previous_style)
{
  if (btk_widget_get_realized (widget) &&
      btk_widget_get_has_window (widget))
    btk_style_set_background (widget->style, widget->window, widget->state);
}

static void
btk_widget_set_style_internal (BtkWidget *widget,
			       BtkStyle	 *style,
			       gboolean   initial_emission)
{
  g_object_ref (widget);
  g_object_freeze_notify (G_OBJECT (widget));

  if (widget->style != style)
    {
      BtkStyle *previous_style;

      if (btk_widget_get_realized (widget))
	{
	  btk_widget_reset_shapes (widget);
	  btk_style_detach (widget->style);
	}
      
      previous_style = widget->style;
      widget->style = style;
      g_object_ref (widget->style);
      
      if (btk_widget_get_realized (widget))
	widget->style = btk_style_attach (widget->style, widget->window);

      btk_widget_update_bango_context (widget);
      g_signal_emit (widget,
		     widget_signals[STYLE_SET],
		     0,
		     initial_emission ? NULL : previous_style);
      g_object_unref (previous_style);

      if (BTK_WIDGET_ANCHORED (widget) && !initial_emission)
	btk_widget_queue_resize (widget);
    }
  else if (initial_emission)
    {
      btk_widget_update_bango_context (widget);
      g_signal_emit (widget,
		     widget_signals[STYLE_SET],
		     0,
		     NULL);
    }
  g_object_notify (G_OBJECT (widget), "style");
  g_object_thaw_notify (G_OBJECT (widget));
  g_object_unref (widget);
}

typedef struct {
  BtkWidget *previous_toplevel;
  BdkScreen *previous_screen;
  BdkScreen *new_screen;
} HierarchyChangedInfo;

static void
do_screen_change (BtkWidget *widget,
		  BdkScreen *old_screen,
		  BdkScreen *new_screen)
{
  if (old_screen != new_screen)
    {
      if (old_screen)
	{
	  BangoContext *context = g_object_get_qdata (G_OBJECT (widget), quark_bango_context);
	  if (context)
	    g_object_set_qdata (G_OBJECT (widget), quark_bango_context, NULL);
	}
      
      _btk_tooltip_hide (widget);
      g_signal_emit (widget, widget_signals[SCREEN_CHANGED], 0, old_screen);
    }
}

static void
btk_widget_propagate_hierarchy_changed_recurse (BtkWidget *widget,
						gpointer   client_data)
{
  HierarchyChangedInfo *info = client_data;
  gboolean new_anchored = btk_widget_is_toplevel (widget) ||
                 (widget->parent && BTK_WIDGET_ANCHORED (widget->parent));

  if (BTK_WIDGET_ANCHORED (widget) != new_anchored)
    {
      g_object_ref (widget);
      
      if (new_anchored)
	BTK_PRIVATE_SET_FLAG (widget, BTK_ANCHORED);
      else
	BTK_PRIVATE_UNSET_FLAG (widget, BTK_ANCHORED);
      
      g_signal_emit (widget, widget_signals[HIERARCHY_CHANGED], 0, info->previous_toplevel);
      do_screen_change (widget, info->previous_screen, info->new_screen);
      
      if (BTK_IS_CONTAINER (widget))
	btk_container_forall (BTK_CONTAINER (widget),
			      btk_widget_propagate_hierarchy_changed_recurse,
			      client_data);
      
      g_object_unref (widget);
    }
}

/**
 * _btk_widget_propagate_hierarchy_changed:
 * @widget: a #BtkWidget
 * @previous_toplevel: Previous toplevel
 * 
 * Propagates changes in the anchored state to a widget and all
 * children, unsetting or setting the %ANCHORED flag, and
 * emitting #BtkWidget::hierarchy-changed.
 **/
void
_btk_widget_propagate_hierarchy_changed (BtkWidget    *widget,
					 BtkWidget    *previous_toplevel)
{
  HierarchyChangedInfo info;

  info.previous_toplevel = previous_toplevel;
  info.previous_screen = previous_toplevel ? btk_widget_get_screen (previous_toplevel) : NULL;

  if (btk_widget_is_toplevel (widget) ||
      (widget->parent && BTK_WIDGET_ANCHORED (widget->parent)))
    info.new_screen = btk_widget_get_screen (widget);
  else
    info.new_screen = NULL;

  if (info.previous_screen)
    g_object_ref (info.previous_screen);
  if (previous_toplevel)
    g_object_ref (previous_toplevel);

  btk_widget_propagate_hierarchy_changed_recurse (widget, &info);

  if (previous_toplevel)
    g_object_unref (previous_toplevel);
  if (info.previous_screen)
    g_object_unref (info.previous_screen);
}

static void
btk_widget_propagate_screen_changed_recurse (BtkWidget *widget,
					     gpointer   client_data)
{
  HierarchyChangedInfo *info = client_data;

  g_object_ref (widget);
  
  do_screen_change (widget, info->previous_screen, info->new_screen);
  
  if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget),
			  btk_widget_propagate_screen_changed_recurse,
			  client_data);
  
  g_object_unref (widget);
}

/**
 * btk_widget_is_composited:
 * @widget: a #BtkWidget
 * 
 * Whether @widget can rely on having its alpha channel
 * drawn correctly. On X11 this function returns whether a
 * compositing manager is running for @widget's screen.
 *
 * Please note that the semantics of this call will change
 * in the future if used on a widget that has a composited
 * window in its hierarchy (as set by bdk_window_set_composited()).
 * 
 * Return value: %TRUE if the widget can rely on its alpha
 * channel being drawn correctly.
 * 
 * Since: 2.10
 */
gboolean
btk_widget_is_composited (BtkWidget *widget)
{
  BdkScreen *screen;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  
  screen = btk_widget_get_screen (widget);
  
  return bdk_screen_is_composited (screen);
}

static void
propagate_composited_changed (BtkWidget *widget,
			      gpointer dummy)
{
  if (BTK_IS_CONTAINER (widget))
    {
      btk_container_forall (BTK_CONTAINER (widget),
			    propagate_composited_changed,
			    NULL);
    }
  
  g_signal_emit (widget, widget_signals[COMPOSITED_CHANGED], 0);
}

void
_btk_widget_propagate_composited_changed (BtkWidget *widget)
{
  propagate_composited_changed (widget, NULL);
}

/**
 * _btk_widget_propagate_screen_changed:
 * @widget: a #BtkWidget
 * @previous_screen: Previous screen
 * 
 * Propagates changes in the screen for a widget to all
 * children, emitting #BtkWidget::screen-changed.
 **/
void
_btk_widget_propagate_screen_changed (BtkWidget    *widget,
				      BdkScreen    *previous_screen)
{
  HierarchyChangedInfo info;

  info.previous_screen = previous_screen;
  info.new_screen = btk_widget_get_screen (widget);

  if (previous_screen)
    g_object_ref (previous_screen);

  btk_widget_propagate_screen_changed_recurse (widget, &info);

  if (previous_screen)
    g_object_unref (previous_screen);
}

static void
reset_rc_styles_recurse (BtkWidget *widget, gpointer data)
{
  if (btk_widget_has_rc_style (widget))
    btk_widget_reset_rc_style (widget);
  
  if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget),
			  reset_rc_styles_recurse,
			  NULL);
}


/**
 * btk_widget_reset_rc_styles:
 * @widget: a #BtkWidget.
 *
 * Reset the styles of @widget and all descendents, so when
 * they are looked up again, they get the correct values
 * for the currently loaded RC file settings.
 *
 * This function is not useful for applications.
 */
void
btk_widget_reset_rc_styles (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  reset_rc_styles_recurse (widget, NULL);
}

/**
 * btk_widget_get_default_style:
 * 
 * Returns the default style used by all widgets initially.
 *
 * Returns: (transfer none): the default style. This #BtkStyle object is owned
 *          by BTK+ and should not be modified or freed.
 */
BtkStyle*
btk_widget_get_default_style (void)
{
  if (!btk_default_style)
    {
      btk_default_style = btk_style_new ();
      g_object_ref (btk_default_style);
    }
  
  return btk_default_style;
}

static BangoContext *
btk_widget_peek_bango_context (BtkWidget *widget)
{
  return g_object_get_qdata (G_OBJECT (widget), quark_bango_context);
}

/**
 * btk_widget_get_bango_context:
 * @widget: a #BtkWidget
 * 
 * Gets a #BangoContext with the appropriate font map, font description,
 * and base direction for this widget. Unlike the context returned
 * by btk_widget_create_bango_context(), this context is owned by
 * the widget (it can be used until the screen for the widget changes
 * or the widget is removed from its toplevel), and will be updated to
 * match any changes to the widget's attributes.
 *
 * If you create and keep a #BangoLayout using this context, you must
 * deal with changes to the context by calling bango_layout_context_changed()
 * on the layout in response to the #BtkWidget::style-set and 
 * #BtkWidget::direction-changed signals for the widget.
 *
 * Return value: (transfer none): the #BangoContext for the widget.
 **/
BangoContext *
btk_widget_get_bango_context (BtkWidget *widget)
{
  BangoContext *context;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  context = g_object_get_qdata (G_OBJECT (widget), quark_bango_context);
  if (!context)
    {
      context = btk_widget_create_bango_context (BTK_WIDGET (widget));
      g_object_set_qdata_full (G_OBJECT (widget),
			       quark_bango_context,
			       context,
			       g_object_unref);
    }

  return context;
}

static void
update_bango_context (BtkWidget    *widget,
		      BangoContext *context)
{
  bango_context_set_font_description (context, widget->style->font_desc);
  bango_context_set_base_dir (context,
			      btk_widget_get_direction (widget) == BTK_TEXT_DIR_LTR ?
			      BANGO_DIRECTION_LTR : BANGO_DIRECTION_RTL);
}

static void
btk_widget_update_bango_context (BtkWidget *widget)
{
  BangoContext *context = btk_widget_peek_bango_context (widget);
  
  if (context)
    {
      BdkScreen *screen;

      update_bango_context (widget, context);

      screen = btk_widget_get_screen_unchecked (widget);
      if (screen)
	{
	  bango_bairo_context_set_resolution (context,
					      bdk_screen_get_resolution (screen));
	  bango_bairo_context_set_font_options (context,
						bdk_screen_get_font_options (screen));
	}
    }
}

/**
 * btk_widget_create_bango_context:
 * @widget: a #BtkWidget
 *
 * Creates a new #BangoContext with the appropriate font map,
 * font description, and base direction for drawing text for
 * this widget. See also btk_widget_get_bango_context().
 *
 * Return value: (transfer full): the new #BangoContext
 **/
BangoContext *
btk_widget_create_bango_context (BtkWidget *widget)
{
  BdkScreen *screen;
  BangoContext *context;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  screen = btk_widget_get_screen_unchecked (widget);
  if (!screen)
    {
      BTK_NOTE (MULTIHEAD,
		g_warning ("btk_widget_create_bango_context ()) called without screen"));

      screen = bdk_screen_get_default ();
    }

  context = bdk_bango_context_get_for_screen (screen);

  update_bango_context (widget, context);
  bango_context_set_language (context, btk_get_default_language ());

  return context;
}

/**
 * btk_widget_create_bango_layout:
 * @widget: a #BtkWidget
 * @text: text to set on the layout (can be %NULL)
 *
 * Creates a new #BangoLayout with the appropriate font map,
 * font description, and base direction for drawing text for
 * this widget.
 *
 * If you keep a #BangoLayout created in this way around, in order to
 * notify the layout of changes to the base direction or font of this
 * widget, you must call bango_layout_context_changed() in response to
 * the #BtkWidget::style-set and #BtkWidget::direction-changed signals
 * for the widget.
 *
 * Return value: (transfer full): the new #BangoLayout
 **/
BangoLayout *
btk_widget_create_bango_layout (BtkWidget   *widget,
				const gchar *text)
{
  BangoLayout *layout;
  BangoContext *context;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  context = btk_widget_get_bango_context (widget);
  layout = bango_layout_new (context);

  if (text)
    bango_layout_set_text (layout, text, -1);

  return layout;
}

/**
 * btk_widget_render_icon:
 * @widget: a #BtkWidget
 * @stock_id: a stock ID
 * @size: (type int) a stock size. A size of (BtkIconSize)-1 means
 *     render at the size of the source and don't scale (if there are
 *     multiple source sizes, BTK+ picks one of the available sizes).
 * @detail: (allow-none): render detail to pass to theme engine
 *
 * A convenience function that uses the theme engine and RC file
 * settings for @widget to look up @stock_id and render it to
 * a pixbuf. @stock_id should be a stock icon ID such as
 * #BTK_STOCK_OPEN or #BTK_STOCK_OK. @size should be a size
 * such as #BTK_ICON_SIZE_MENU. @detail should be a string that
 * identifies the widget or code doing the rendering, so that
 * theme engines can special-case rendering for that widget or code.
 *
 * The pixels in the returned #BdkPixbuf are shared with the rest of
 * the application and should not be modified. The pixbuf should be freed
 * after use with g_object_unref().
 *
 * Return value: (transfer full): a new pixbuf, or %NULL if the
 *     stock ID wasn't known
 **/
BdkPixbuf*
btk_widget_render_icon (BtkWidget      *widget,
                        const gchar    *stock_id,
                        BtkIconSize     size,
                        const gchar    *detail)
{
  BtkIconSet *icon_set;
  BdkPixbuf *retval;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (size > BTK_ICON_SIZE_INVALID || size == -1, NULL);
  
  btk_widget_ensure_style (widget);
  
  icon_set = btk_style_lookup_icon_set (widget->style, stock_id);

  if (icon_set == NULL)
    return NULL;

  retval = btk_icon_set_render_icon (icon_set,
                                     widget->style,
                                     btk_widget_get_direction (widget),
                                     btk_widget_get_state (widget),
                                     size,
                                     widget,
                                     detail);

  return retval;
}

/**
 * btk_widget_set_parent_window:
 * @widget: a #BtkWidget.
 * @parent_window: the new parent window.
 *  
 * Sets a non default parent window for @widget.
 **/
void
btk_widget_set_parent_window   (BtkWidget           *widget,
				BdkWindow           *parent_window)
{
  BdkWindow *old_parent_window;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  old_parent_window = g_object_get_qdata (G_OBJECT (widget),
					  quark_parent_window);

  if (parent_window != old_parent_window)
    {
      g_object_set_qdata (G_OBJECT (widget), quark_parent_window, 
			  parent_window);
      if (old_parent_window)
	g_object_unref (old_parent_window);
      if (parent_window)
	g_object_ref (parent_window);
    }
}

/**
 * btk_widget_get_parent_window:
 * @widget: a #BtkWidget.
 *
 * Gets @widget's parent window.
 *
 * Returns: (transfer none): the parent window of @widget.
 **/
BdkWindow *
btk_widget_get_parent_window (BtkWidget *widget)
{
  BdkWindow *parent_window;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  parent_window = g_object_get_qdata (G_OBJECT (widget), quark_parent_window);

  return (parent_window != NULL) ? parent_window :
	 (widget->parent != NULL) ? widget->parent->window : NULL;
}


/**
 * btk_widget_set_child_visible:
 * @widget: a #BtkWidget
 * @is_visible: if %TRUE, @widget should be mapped along with its parent.
 *
 * Sets whether @widget should be mapped along with its when its parent
 * is mapped and @widget has been shown with btk_widget_show(). 
 *
 * The child visibility can be set for widget before it is added to
 * a container with btk_widget_set_parent(), to avoid mapping
 * children unnecessary before immediately unmapping them. However
 * it will be reset to its default state of %TRUE when the widget
 * is removed from a container.
 * 
 * Note that changing the child visibility of a widget does not
 * queue a resize on the widget. Most of the time, the size of
 * a widget is computed from all visible children, whether or
 * not they are mapped. If this is not the case, the container
 * can queue a resize itself.
 *
 * This function is only useful for container implementations and
 * never should be called by an application.
 **/
void
btk_widget_set_child_visible (BtkWidget *widget,
			      gboolean   is_visible)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (!btk_widget_is_toplevel (widget));

  g_object_ref (widget);

  if (is_visible)
    BTK_PRIVATE_SET_FLAG (widget, BTK_CHILD_VISIBLE);
  else
    {
      BtkWidget *toplevel;
      
      BTK_PRIVATE_UNSET_FLAG (widget, BTK_CHILD_VISIBLE);

      toplevel = btk_widget_get_toplevel (widget);
      if (toplevel != widget && btk_widget_is_toplevel (toplevel))
	_btk_window_unset_focus_and_default (BTK_WINDOW (toplevel), widget);
    }

  if (widget->parent && btk_widget_get_realized (widget->parent))
    {
      if (btk_widget_get_mapped (widget->parent) &&
	  BTK_WIDGET_CHILD_VISIBLE (widget) &&
	  btk_widget_get_visible (widget))
	btk_widget_map (widget);
      else
	btk_widget_unmap (widget);
    }

  g_object_unref (widget);
}

/**
 * btk_widget_get_child_visible:
 * @widget: a #BtkWidget
 * 
 * Gets the value set with btk_widget_set_child_visible().
 * If you feel a need to use this function, your code probably
 * needs reorganization. 
 *
 * This function is only useful for container implementations and
 * never should be called by an application.
 *
 * Return value: %TRUE if the widget is mapped with the parent.
 **/
gboolean
btk_widget_get_child_visible (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  
  return BTK_WIDGET_CHILD_VISIBLE (widget);
}

static BdkScreen *
btk_widget_get_screen_unchecked (BtkWidget *widget)
{
  BtkWidget *toplevel;
  
  toplevel = btk_widget_get_toplevel (widget);

  if (btk_widget_is_toplevel (toplevel))
    {
      if (BTK_IS_WINDOW (toplevel))
	return BTK_WINDOW (toplevel)->screen;
      else if (BTK_IS_INVISIBLE (toplevel))
	return BTK_INVISIBLE (widget)->screen;
    }

  return NULL;
}

/**
 * btk_widget_get_screen:
 * @widget: a #BtkWidget
 * 
 * Get the #BdkScreen from the toplevel window associated with
 * this widget. This function can only be called after the widget
 * has been added to a widget hierarchy with a #BtkWindow
 * at the top.
 *
 * In general, you should only create screen specific
 * resources when a widget has been realized, and you should
 * free those resources when the widget is unrealized.
 *
 * Return value: (transfer none): the #BdkScreen for the toplevel for this widget.
 *
 * Since: 2.2
 **/
BdkScreen*
btk_widget_get_screen (BtkWidget *widget)
{
  BdkScreen *screen;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  screen = btk_widget_get_screen_unchecked (widget);

  if (screen)
    return screen;
  else
    {
#if 0
      g_warning (B_STRLOC ": Can't get associated screen"
		 " for a widget unless it is inside a toplevel BtkWindow\n"
		 " widget type is %s associated top level type is %s",
		 g_type_name (G_OBJECT_TYPE(G_OBJECT (widget))),
		 g_type_name (G_OBJECT_TYPE(G_OBJECT (toplevel))));
#endif
      return bdk_screen_get_default ();
    }
}

/**
 * btk_widget_has_screen:
 * @widget: a #BtkWidget
 * 
 * Checks whether there is a #BdkScreen is associated with
 * this widget. All toplevel widgets have an associated
 * screen, and all widgets added into a hierarchy with a toplevel
 * window at the top.
 * 
 * Return value: %TRUE if there is a #BdkScreen associcated
 *   with the widget.
 *
 * Since: 2.2
 **/
gboolean
btk_widget_has_screen (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  return (btk_widget_get_screen_unchecked (widget) != NULL);
}

/**
 * btk_widget_get_display:
 * @widget: a #BtkWidget
 * 
 * Get the #BdkDisplay for the toplevel window associated with
 * this widget. This function can only be called after the widget
 * has been added to a widget hierarchy with a #BtkWindow at the top.
 *
 * In general, you should only create display specific
 * resources when a widget has been realized, and you should
 * free those resources when the widget is unrealized.
 *
 * Return value: (transfer none): the #BdkDisplay for the toplevel for this widget.
 *
 * Since: 2.2
 **/
BdkDisplay*
btk_widget_get_display (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  return bdk_screen_get_display (btk_widget_get_screen (widget));
}

/**
 * btk_widget_get_root_window:
 * @widget: a #BtkWidget
 * 
 * Get the root window where this widget is located. This function can
 * only be called after the widget has been added to a widget
 * hierarchy with #BtkWindow at the top.
 *
 * The root window is useful for such purposes as creating a popup
 * #BdkWindow associated with the window. In general, you should only
 * create display specific resources when a widget has been realized,
 * and you should free those resources when the widget is unrealized.
 *
 * Return value: (transfer none): the #BdkWindow root window for the toplevel for this widget.
 *
 * Since: 2.2
 **/
BdkWindow*
btk_widget_get_root_window (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  return bdk_screen_get_root_window (btk_widget_get_screen (widget));
}

/**
 * btk_widget_child_focus:
 * @widget: a #BtkWidget
 * @direction: direction of focus movement
 *
 * This function is used by custom widget implementations; if you're
 * writing an app, you'd use btk_widget_grab_focus() to move the focus
 * to a particular widget, and btk_container_set_focus_chain() to
 * change the focus tab order. So you may want to investigate those
 * functions instead.
 * 
 * btk_widget_child_focus() is called by containers as the user moves
 * around the window using keyboard shortcuts. @direction indicates
 * what kind of motion is taking place (up, down, left, right, tab
 * forward, tab backward). btk_widget_child_focus() emits the
 * #BtkWidget::focus signal; widgets override the default handler
 * for this signal in order to implement appropriate focus behavior.
 *
 * The default ::focus handler for a widget should return %TRUE if
 * moving in @direction left the focus on a focusable location inside
 * that widget, and %FALSE if moving in @direction moved the focus
 * outside the widget. If returning %TRUE, widgets normally
 * call btk_widget_grab_focus() to place the focus accordingly;
 * if returning %FALSE, they don't modify the current focus location.
 * 
 * This function replaces btk_container_focus() from BTK+ 1.2.  
 * It was necessary to check that the child was visible, sensitive, 
 * and focusable before calling btk_container_focus(). 
 * btk_widget_child_focus() returns %FALSE if the widget is not 
 * currently in a focusable state, so there's no need for those checks.
 * 
 * Return value: %TRUE if focus ended up inside @widget
 **/
gboolean
btk_widget_child_focus (BtkWidget       *widget,
                        BtkDirectionType direction)
{
  gboolean return_val;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  if (!btk_widget_get_visible (widget) ||
      !btk_widget_is_sensitive (widget))
    return FALSE;

  /* child widgets must set CAN_FOCUS, containers
   * don't have to though.
   */
  if (!BTK_IS_CONTAINER (widget) &&
      !btk_widget_get_can_focus (widget))
    return FALSE;
  
  g_signal_emit (widget,
		 widget_signals[FOCUS],
		 0,
		 direction, &return_val);

  return return_val;
}

/**
 * btk_widget_keynav_failed:
 * @widget: a #BtkWidget
 * @direction: direction of focus movement
 *
 * This function should be called whenever keyboard navigation within
 * a single widget hits a boundary. The function emits the
 * #BtkWidget::keynav-failed signal on the widget and its return
 * value should be interpreted in a way similar to the return value of
 * btk_widget_child_focus():
 *
 * When %TRUE is returned, stay in the widget, the failed keyboard
 * navigation is Ok and/or there is nowhere we can/should move the
 * focus to.
 *
 * When %FALSE is returned, the caller should continue with keyboard
 * navigation outside the widget, e.g. by calling
 * btk_widget_child_focus() on the widget's toplevel.
 *
 * The default ::keynav-failed handler returns %TRUE for 
 * %BTK_DIR_TAB_FORWARD and %BTK_DIR_TAB_BACKWARD. For the other 
 * values of #BtkDirectionType, it looks at the 
 * #BtkSettings:btk-keynav-cursor-only setting and returns %FALSE 
 * if the setting is %TRUE. This way the entire user interface
 * becomes cursor-navigatable on input devices such as mobile phones
 * which only have cursor keys but no tab key.
 *
 * Whenever the default handler returns %TRUE, it also calls
 * btk_widget_error_bell() to notify the user of the failed keyboard
 * navigation.
 *
 * A use case for providing an own implementation of ::keynav-failed 
 * (either by connecting to it or by overriding it) would be a row of
 * #BtkEntry widgets where the user should be able to navigate the
 * entire row with the cursor keys, as e.g. known from user interfaces 
 * that require entering license keys.
 *
 * Return value: %TRUE if stopping keyboard navigation is fine, %FALSE
 *               if the emitting widget should try to handle the keyboard
 *               navigation attempt in its parent container(s).
 *
 * Since: 2.12
 **/
gboolean
btk_widget_keynav_failed (BtkWidget        *widget,
                          BtkDirectionType  direction)
{
  gboolean return_val;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  g_signal_emit (widget, widget_signals[KEYNAV_FAILED], 0,
		 direction, &return_val);

  return return_val;
}

/**
 * btk_widget_error_bell:
 * @widget: a #BtkWidget
 *
 * Notifies the user about an input-related error on this widget. 
 * If the #BtkSettings:btk-error-bell setting is %TRUE, it calls
 * bdk_window_beep(), otherwise it does nothing.
 *
 * Note that the effect of bdk_window_beep() can be configured in many
 * ways, depending on the windowing backend and the desktop environment
 * or window manager that is used.
 *
 * Since: 2.12
 **/
void
btk_widget_error_bell (BtkWidget *widget)
{
  BtkSettings* settings;
  gboolean beep;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  settings = btk_widget_get_settings (widget);
  if (!settings)
    return;

  g_object_get (settings,
                "btk-error-bell", &beep,
                NULL);

  if (beep && widget->window)
    bdk_window_beep (widget->window);
}

/**
 * btk_widget_set_uposition:
 * @widget: a #BtkWidget
 * @x: x position; -1 to unset x; -2 to leave x unchanged
 * @y: y position; -1 to unset y; -2 to leave y unchanged
 * 
 *
 * Sets the position of a widget. The funny "u" in the name comes from
 * the "user position" hint specified by the X Window System, and
 * exists for legacy reasons. This function doesn't work if a widget
 * is inside a container; it's only really useful on #BtkWindow.
 *
 * Don't use this function to center dialogs over the main application
 * window; most window managers will do the centering on your behalf
 * if you call btk_window_set_transient_for(), and it's really not
 * possible to get the centering to work correctly in all cases from
 * application code. But if you insist, use btk_window_set_position()
 * to set #BTK_WIN_POS_CENTER_ON_PARENT, don't do the centering
 * manually.
 *
 * Note that although @x and @y can be individually unset, the position
 * is not honoured unless both @x and @y are set.
 **/
void
btk_widget_set_uposition (BtkWidget *widget,
			  gint	     x,
			  gint	     y)
{
  /* FIXME this function is the only place that aux_info->x and
   * aux_info->y are even used I believe, and this function is
   * deprecated. Should be cleaned up.
   *
   * (Actually, size_allocate uses them) -Yosh
   */
  
  BtkWidgetAuxInfo *aux_info;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  aux_info =_btk_widget_get_aux_info (widget, TRUE);
  
  if (x > -2)
    {
      if (x == -1)
	aux_info->x_set = FALSE;
      else
	{
	  aux_info->x_set = TRUE;
	  aux_info->x = x;
	}
    }

  if (y > -2)
    {
      if (y == -1)
	aux_info->y_set = FALSE;
      else
	{
	  aux_info->y_set = TRUE;
	  aux_info->y = y;
	}
    }

  if (BTK_IS_WINDOW (widget) && aux_info->x_set && aux_info->y_set)
    _btk_window_reposition (BTK_WINDOW (widget), aux_info->x, aux_info->y);
  
  if (btk_widget_get_visible (widget) && widget->parent)
    btk_widget_size_allocate (widget, &widget->allocation);
}

static void
btk_widget_set_usize_internal (BtkWidget *widget,
			       gint       width,
			       gint       height)
{
  BtkWidgetAuxInfo *aux_info;
  gboolean changed = FALSE;
  
  g_object_freeze_notify (G_OBJECT (widget));

  aux_info = _btk_widget_get_aux_info (widget, TRUE);
  
  if (width > -2 && aux_info->width != width)
    {
      g_object_notify (G_OBJECT (widget), "width-request");
      aux_info->width = width;
      changed = TRUE;
    }
  if (height > -2 && aux_info->height != height)
    {
      g_object_notify (G_OBJECT (widget), "height-request");  
      aux_info->height = height;
      changed = TRUE;
    }
  
  if (btk_widget_get_visible (widget) && changed)
    btk_widget_queue_resize (widget);

  g_object_thaw_notify (G_OBJECT (widget));
}

/**
 * btk_widget_set_usize:
 * @widget: a #BtkWidget
 * @width: minimum width, or -1 to unset
 * @height: minimum height, or -1 to unset
 *
 * Sets the minimum size of a widget; that is, the widget's size
 * request will be @width by @height. You can use this function to
 * force a widget to be either larger or smaller than it is. The
 * strange "usize" name dates from the early days of BTK+, and derives
 * from X Window System terminology. In many cases,
 * btk_window_set_default_size() is a better choice for toplevel
 * windows than this function; setting the default size will still
 * allow users to shrink the window. Setting the usize will force them
 * to leave the window at least as large as the usize. When dealing
 * with window sizes, btk_window_set_geometry_hints() can be a useful
 * function as well.
 * 
 * Note the inherent danger of setting any fixed size - themes,
 * translations into other languages, different fonts, and user action
 * can all change the appropriate size for a given widget. So, it's
 * basically impossible to hardcode a size that will always be
 * correct.
 * 
 * Deprecated: 2.2: Use btk_widget_set_size_request() instead.
 **/
void
btk_widget_set_usize (BtkWidget *widget,
		      gint	 width,
		      gint	 height)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  btk_widget_set_usize_internal (widget, width, height);
}

/**
 * btk_widget_set_size_request:
 * @widget: a #BtkWidget
 * @width: width @widget should request, or -1 to unset
 * @height: height @widget should request, or -1 to unset
 *
 * Sets the minimum size of a widget; that is, the widget's size
 * request will be @width by @height. You can use this function to
 * force a widget to be either larger or smaller than it normally
 * would be.
 *
 * In most cases, btk_window_set_default_size() is a better choice for
 * toplevel windows than this function; setting the default size will
 * still allow users to shrink the window. Setting the size request
 * will force them to leave the window at least as large as the size
 * request. When dealing with window sizes,
 * btk_window_set_geometry_hints() can be a useful function as well.
 * 
 * Note the inherent danger of setting any fixed size - themes,
 * translations into other languages, different fonts, and user action
 * can all change the appropriate size for a given widget. So, it's
 * basically impossible to hardcode a size that will always be
 * correct.
 *
 * The size request of a widget is the smallest size a widget can
 * accept while still functioning well and drawing itself correctly.
 * However in some strange cases a widget may be allocated less than
 * its requested size, and in many cases a widget may be allocated more
 * space than it requested.
 *
 * If the size request in a given direction is -1 (unset), then
 * the "natural" size request of the widget will be used instead.
 *
 * Widgets can't actually be allocated a size less than 1 by 1, but
 * you can pass 0,0 to this function to mean "as small as possible."
 **/
void
btk_widget_set_size_request (BtkWidget *widget,
                             gint       width,
                             gint       height)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  if (width == 0)
    width = 1;
  if (height == 0)
    height = 1;
  
  btk_widget_set_usize_internal (widget, width, height);
}


/**
 * btk_widget_get_size_request:
 * @widget: a #BtkWidget
 * @width: (out) (allow-none): return location for width, or %NULL
 * @height: (out) (allow-none): return location for height, or %NULL
 *
 * Gets the size request that was explicitly set for the widget using
 * btk_widget_set_size_request(). A value of -1 stored in @width or
 * @height indicates that that dimension has not been set explicitly
 * and the natural requisition of the widget will be used intead. See
 * btk_widget_set_size_request(). To get the size a widget will
 * actually use, call btk_widget_size_request() instead of
 * this function.
 **/
void
btk_widget_get_size_request (BtkWidget *widget,
                             gint      *width,
                             gint      *height)
{
  BtkWidgetAuxInfo *aux_info;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  aux_info = _btk_widget_get_aux_info (widget, FALSE);

  if (width)
    *width = aux_info ? aux_info->width : -1;

  if (height)
    *height = aux_info ? aux_info->height : -1;
}

/**
 * btk_widget_set_events:
 * @widget: a #BtkWidget
 * @events: event mask
 *
 * Sets the event mask (see #BdkEventMask) for a widget. The event
 * mask determines which events a widget will receive. Keep in mind
 * that different widgets have different default event masks, and by
 * changing the event mask you may disrupt a widget's functionality,
 * so be careful. This function must be called while a widget is
 * unrealized. Consider btk_widget_add_events() for widgets that are
 * already realized, or if you want to preserve the existing event
 * mask. This function can't be used with #BTK_NO_WINDOW widgets;
 * to get events on those widgets, place them inside a #BtkEventBox
 * and receive events on the event box.
 **/
void
btk_widget_set_events (BtkWidget *widget,
		       gint	  events)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (!btk_widget_get_realized (widget));
  
  g_object_set_qdata (G_OBJECT (widget), quark_event_mask,
                      GINT_TO_POINTER (events));
  g_object_notify (G_OBJECT (widget), "events");
}

static void
btk_widget_add_events_internal (BtkWidget *widget,
				gint       events,
				GList     *window_list)
{
  GList *l;

  for (l = window_list; l != NULL; l = l->next)
    {
      BdkWindow *window = l->data;
      gpointer user_data;

      bdk_window_get_user_data (window, &user_data);
      if (user_data == widget)
	{
	  GList *children;

	  bdk_window_set_events (window, bdk_window_get_events (window) | events);

	  children = bdk_window_get_children (window);
	  btk_widget_add_events_internal (widget, events, children);
	  g_list_free (children);
	}
    }
}

/**
 * btk_widget_add_events:
 * @widget: a #BtkWidget
 * @events: an event mask, see #BdkEventMask
 *
 * Adds the events in the bitfield @events to the event mask for
 * @widget. See btk_widget_set_events() for details.
 **/
void
btk_widget_add_events (BtkWidget *widget,
		       gint	  events)
{
  gint old_events;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  old_events = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget), quark_event_mask));
  g_object_set_qdata (G_OBJECT (widget), quark_event_mask,
                      GINT_TO_POINTER (old_events | events));

  if (btk_widget_get_realized (widget))
    {
      GList *window_list;

      if (!btk_widget_get_has_window (widget))
	window_list = bdk_window_get_children (widget->window);
      else
	window_list = g_list_prepend (NULL, widget->window);

      btk_widget_add_events_internal (widget, events, window_list);

      g_list_free (window_list);
    }

  g_object_notify (G_OBJECT (widget), "events");
}

/**
 * btk_widget_set_extension_events:
 * @widget: a #BtkWidget
 * @mode: bitfield of extension events to receive
 *
 * Sets the extension events mask to @mode. See #BdkExtensionMode
 * and bdk_input_set_extension_events().
 **/
void
btk_widget_set_extension_events (BtkWidget *widget,
				 BdkExtensionMode mode)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (btk_widget_get_realized (widget))
    btk_widget_set_extension_events_internal (widget, mode, NULL);

  g_object_set_qdata (G_OBJECT (widget), quark_extension_event_mode,
                      GINT_TO_POINTER (mode));
  g_object_notify (G_OBJECT (widget), "extension-events");
}

/**
 * btk_widget_get_toplevel:
 * @widget: a #BtkWidget
 * 
 * This function returns the topmost widget in the container hierarchy
 * @widget is a part of. If @widget has no parent widgets, it will be
 * returned as the topmost widget. No reference will be added to the
 * returned widget; it should not be unreferenced.
 *
 * Note the difference in behavior vs. btk_widget_get_ancestor();
 * <literal>btk_widget_get_ancestor (widget, BTK_TYPE_WINDOW)</literal> 
 * would return
 * %NULL if @widget wasn't inside a toplevel window, and if the
 * window was inside a #BtkWindow-derived widget which was in turn
 * inside the toplevel #BtkWindow. While the second case may
 * seem unlikely, it actually happens when a #BtkPlug is embedded
 * inside a #BtkSocket within the same application.
 * 
 * To reliably find the toplevel #BtkWindow, use
 * btk_widget_get_toplevel() and check if the %TOPLEVEL flags
 * is set on the result.
 * |[
 *  BtkWidget *toplevel = btk_widget_get_toplevel (widget);
 *  if (btk_widget_is_toplevel (toplevel))
 *    {
 *      /&ast; Perform action on toplevel. &ast;/
 *    }
 * ]|
 *
 * Return value: (transfer none): the topmost ancestor of @widget, or @widget itself
 *    if there's no ancestor.
 **/
BtkWidget*
btk_widget_get_toplevel (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  while (widget->parent)
    widget = widget->parent;
  
  return widget;
}

/**
 * btk_widget_get_ancestor:
 * @widget: a #BtkWidget
 * @widget_type: ancestor type
 * 
 * Gets the first ancestor of @widget with type @widget_type. For example,
 * <literal>btk_widget_get_ancestor (widget, BTK_TYPE_BOX)</literal> gets 
 * the first #BtkBox that's an ancestor of @widget. No reference will be 
 * added to the returned widget; it should not be unreferenced. See note 
 * about checking for a toplevel #BtkWindow in the docs for 
 * btk_widget_get_toplevel().
 * 
 * Note that unlike btk_widget_is_ancestor(), btk_widget_get_ancestor() 
 * considers @widget to be an ancestor of itself.
 *
 * Return value: (transfer none): the ancestor widget, or %NULL if not found
 **/
BtkWidget*
btk_widget_get_ancestor (BtkWidget *widget,
			 GType      widget_type)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  while (widget && !g_type_is_a (G_OBJECT_TYPE (widget), widget_type))
    widget = widget->parent;
  
  if (!(widget && g_type_is_a (G_OBJECT_TYPE (widget), widget_type)))
    return NULL;
  
  return widget;
}

/**
 * btk_widget_get_colormap:
 * @widget: a #BtkWidget
 * 
 * Gets the colormap that will be used to render @widget. No reference will
 * be added to the returned colormap; it should not be unreferenced.
 *
 * Return value: (transfer none): the colormap used by @widget
 **/
BdkColormap*
btk_widget_get_colormap (BtkWidget *widget)
{
  BdkColormap *colormap;
  BtkWidget *tmp_widget;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  if (widget->window)
    {
      colormap = bdk_drawable_get_colormap (widget->window);
      /* If window was destroyed previously, we'll get NULL here */
      if (colormap)
	return colormap;
    }

  tmp_widget = widget;
  while (tmp_widget)
    {
      colormap = g_object_get_qdata (G_OBJECT (tmp_widget), quark_colormap);
      if (colormap)
	return colormap;

      tmp_widget= tmp_widget->parent;
    }

  return bdk_screen_get_default_colormap (btk_widget_get_screen (widget));
}

/**
 * btk_widget_get_visual:
 * @widget: a #BtkWidget
 * 
 * Gets the visual that will be used to render @widget.
 *
 * Return value: (transfer none): the visual for @widget
 **/
BdkVisual*
btk_widget_get_visual (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  return bdk_colormap_get_visual (btk_widget_get_colormap (widget));
}

/**
 * btk_widget_get_settings:
 * @widget: a #BtkWidget
 * 
 * Gets the settings object holding the settings (global property
 * settings, RC file information, etc) used for this widget.
 *
 * Note that this function can only be called when the #BtkWidget
 * is attached to a toplevel, since the settings object is specific
 * to a particular #BdkScreen.
 *
 * Return value: (transfer none): the relevant #BtkSettings object
 **/
BtkSettings*
btk_widget_get_settings (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  
  return btk_settings_get_for_screen (btk_widget_get_screen (widget));
}

/**
 * btk_widget_set_colormap:
 * @widget: a #BtkWidget
 * @colormap: a colormap
 *
 * Sets the colormap for the widget to the given value. Widget must not
 * have been previously realized. This probably should only be used
 * from an <function>init()</function> function (i.e. from the constructor 
 * for the widget).
 **/
void
btk_widget_set_colormap (BtkWidget   *widget,
                         BdkColormap *colormap)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (!btk_widget_get_realized (widget));
  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  g_object_ref (colormap);
  
  g_object_set_qdata_full (G_OBJECT (widget), 
			   quark_colormap,
			   colormap,
			   g_object_unref);
}

/**
 * btk_widget_get_events:
 * @widget: a #BtkWidget
 * 
 * Returns the event mask for the widget (a bitfield containing flags
 * from the #BdkEventMask enumeration). These are the events that the widget
 * will receive.
 * 
 * Return value: event mask for @widget
 **/
gint
btk_widget_get_events (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), 0);

  return GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget), quark_event_mask));
}

/**
 * btk_widget_get_extension_events:
 * @widget: a #BtkWidget
 * 
 * Retrieves the extension events the widget will receive; see
 * bdk_input_set_extension_events().
 * 
 * Return value: extension events for @widget
 **/
BdkExtensionMode
btk_widget_get_extension_events (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), 0);

  return GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (widget), quark_extension_event_mode));
}

/**
 * btk_widget_get_pointer:
 * @widget: a #BtkWidget
 * @x: (out) (allow-none): return location for the X coordinate, or %NULL
 * @y: (out) (allow-none): return location for the Y coordinate, or %NULL
 *
 * Obtains the location of the mouse pointer in widget coordinates.
 * Widget coordinates are a bit odd; for historical reasons, they are
 * defined as @widget->window coordinates for widgets that are not
 * #BTK_NO_WINDOW widgets, and are relative to @widget->allocation.x,
 * @widget->allocation.y for widgets that are #BTK_NO_WINDOW widgets.
 **/
void
btk_widget_get_pointer (BtkWidget *widget,
			gint	  *x,
			gint	  *y)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  if (x)
    *x = -1;
  if (y)
    *y = -1;
  
  if (btk_widget_get_realized (widget))
    {
      bdk_window_get_pointer (widget->window, x, y, NULL);
      
      if (!btk_widget_get_has_window (widget))
	{
	  if (x)
	    *x -= widget->allocation.x;
	  if (y)
	    *y -= widget->allocation.y;
	}
    }
}

/**
 * btk_widget_is_ancestor:
 * @widget: a #BtkWidget
 * @ancestor: another #BtkWidget
 * 
 * Determines whether @widget is somewhere inside @ancestor, possibly with
 * intermediate containers.
 * 
 * Return value: %TRUE if @ancestor contains @widget as a child, 
 *    grandchild, great grandchild, etc.
 **/
gboolean
btk_widget_is_ancestor (BtkWidget *widget,
			BtkWidget *ancestor)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (ancestor != NULL, FALSE);
  
  while (widget)
    {
      if (widget->parent == ancestor)
	return TRUE;
      widget = widget->parent;
    }
  
  return FALSE;
}

static GQuark quark_composite_name = 0;

/**
 * btk_widget_set_composite_name:
 * @widget: a #BtkWidget.
 * @name: the name to set
 * 
 * Sets a widgets composite name. The widget must be
 * a composite child of its parent; see btk_widget_push_composite_child().
 **/
void
btk_widget_set_composite_name (BtkWidget   *widget,
			       const gchar *name)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail ((BTK_OBJECT_FLAGS (widget) & BTK_COMPOSITE_CHILD) != 0);
  g_return_if_fail (name != NULL);

  if (!quark_composite_name)
    quark_composite_name = g_quark_from_static_string ("btk-composite-name");

  g_object_set_qdata_full (G_OBJECT (widget),
			   quark_composite_name,
			   g_strdup (name),
			   g_free);
}

/**
 * btk_widget_get_composite_name:
 * @widget: a #BtkWidget
 *
 * Obtains the composite name of a widget. 
 *
 * Returns: the composite name of @widget, or %NULL if @widget is not
 *   a composite child. The string should be freed when it is no 
 *   longer needed.
 **/
gchar*
btk_widget_get_composite_name (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  if (((BTK_OBJECT_FLAGS (widget) & BTK_COMPOSITE_CHILD) != 0) && widget->parent)
    return _btk_container_child_composite_name (BTK_CONTAINER (widget->parent),
					       widget);
  else
    return NULL;
}

/**
 * btk_widget_push_composite_child:
 * 
 * Makes all newly-created widgets as composite children until
 * the corresponding btk_widget_pop_composite_child() call.
 * 
 * A composite child is a child that's an implementation detail of the
 * container it's inside and should not be visible to people using the
 * container. Composite children aren't treated differently by BTK (but
 * see btk_container_foreach() vs. btk_container_forall()), but e.g. GUI 
 * builders might want to treat them in a different way.
 * 
 * Here is a simple example:
 * |[
 *   btk_widget_push_composite_child ();
 *   scrolled_window->hscrollbar = btk_hscrollbar_new (hadjustment);
 *   btk_widget_set_composite_name (scrolled_window->hscrollbar, "hscrollbar");
 *   btk_widget_pop_composite_child ();
 *   btk_widget_set_parent (scrolled_window->hscrollbar, 
 *                          BTK_WIDGET (scrolled_window));
 *   g_object_ref (scrolled_window->hscrollbar);
 * ]|
 **/
void
btk_widget_push_composite_child (void)
{
  composite_child_stack++;
}

/**
 * btk_widget_pop_composite_child:
 *
 * Cancels the effect of a previous call to btk_widget_push_composite_child().
 **/ 
void
btk_widget_pop_composite_child (void)
{
  if (composite_child_stack)
    composite_child_stack--;
}

/**
 * btk_widget_push_colormap:
 * @cmap: a #BdkColormap
 *
 * Pushes @cmap onto a global stack of colormaps; the topmost
 * colormap on the stack will be used to create all widgets.
 * Remove @cmap with btk_widget_pop_colormap(). There's little
 * reason to use this function.
 **/
void
btk_widget_push_colormap (BdkColormap *cmap)
{
  g_return_if_fail (!cmap || BDK_IS_COLORMAP (cmap));

  colormap_stack = g_slist_prepend (colormap_stack, cmap);
}

/**
 * btk_widget_pop_colormap:
 *
 * Removes a colormap pushed with btk_widget_push_colormap().
 **/
void
btk_widget_pop_colormap (void)
{
  if (colormap_stack)
    colormap_stack = g_slist_delete_link (colormap_stack, colormap_stack);
}

/**
 * btk_widget_set_default_colormap:
 * @colormap: a #BdkColormap
 * 
 * Sets the default colormap to use when creating widgets.
 * btk_widget_push_colormap() is a better function to use if
 * you only want to affect a few widgets, rather than all widgets.
 **/
void
btk_widget_set_default_colormap (BdkColormap *colormap)
{
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  
  bdk_screen_set_default_colormap (bdk_colormap_get_screen (colormap),
				   colormap);
}

/**
 * btk_widget_get_default_colormap:
 * 
 * Obtains the default colormap used to create widgets.
 *
 * Return value: (transfer none): default widget colormap
 **/
BdkColormap*
btk_widget_get_default_colormap (void)
{
  return bdk_screen_get_default_colormap (bdk_screen_get_default ());
}

/**
 * btk_widget_get_default_visual:
 * 
 * Obtains the visual of the default colormap. Not really useful;
 * used to be useful before bdk_colormap_get_visual() existed.
 *
 * Return value: (transfer none): visual of the default colormap
 **/
BdkVisual*
btk_widget_get_default_visual (void)
{
  return bdk_colormap_get_visual (btk_widget_get_default_colormap ());
}

static void
btk_widget_emit_direction_changed (BtkWidget        *widget,
				   BtkTextDirection  old_dir)
{
  btk_widget_update_bango_context (widget);
  
  g_signal_emit (widget, widget_signals[DIRECTION_CHANGED], 0, old_dir);
}

/**
 * btk_widget_set_direction:
 * @widget: a #BtkWidget
 * @dir:    the new direction
 * 
 * Sets the reading direction on a particular widget. This direction
 * controls the primary direction for widgets containing text,
 * and also the direction in which the children of a container are
 * packed. The ability to set the direction is present in order
 * so that correct localization into languages with right-to-left
 * reading directions can be done. Generally, applications will
 * let the default reading direction present, except for containers
 * where the containers are arranged in an order that is explicitely
 * visual rather than logical (such as buttons for text justification).
 *
 * If the direction is set to %BTK_TEXT_DIR_NONE, then the value
 * set by btk_widget_set_default_direction() will be used.
 **/
void
btk_widget_set_direction (BtkWidget        *widget,
			  BtkTextDirection  dir)
{
  BtkTextDirection old_dir;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (dir >= BTK_TEXT_DIR_NONE && dir <= BTK_TEXT_DIR_RTL);

  old_dir = btk_widget_get_direction (widget);
  
  if (dir == BTK_TEXT_DIR_NONE)
    BTK_PRIVATE_UNSET_FLAG (widget, BTK_DIRECTION_SET);
  else
    {
      BTK_PRIVATE_SET_FLAG (widget, BTK_DIRECTION_SET);
      if (dir == BTK_TEXT_DIR_LTR)
	BTK_PRIVATE_SET_FLAG (widget, BTK_DIRECTION_LTR);
      else
	BTK_PRIVATE_UNSET_FLAG (widget, BTK_DIRECTION_LTR);
    }

  if (old_dir != btk_widget_get_direction (widget))
    btk_widget_emit_direction_changed (widget, old_dir);
}

/**
 * btk_widget_get_direction:
 * @widget: a #BtkWidget
 * 
 * Gets the reading direction for a particular widget. See
 * btk_widget_set_direction().
 * 
 * Return value: the reading direction for the widget.
 **/
BtkTextDirection
btk_widget_get_direction (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), BTK_TEXT_DIR_LTR);
  
  if (BTK_WIDGET_DIRECTION_SET (widget))
    return BTK_WIDGET_DIRECTION_LTR (widget) ? BTK_TEXT_DIR_LTR : BTK_TEXT_DIR_RTL;
  else
    return btk_default_direction;
}

static void
btk_widget_set_default_direction_recurse (BtkWidget *widget, gpointer data)
{
  BtkTextDirection old_dir = GPOINTER_TO_UINT (data);

  g_object_ref (widget);
  
  if (!BTK_WIDGET_DIRECTION_SET (widget))
    btk_widget_emit_direction_changed (widget, old_dir);
  
  if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget),
			  btk_widget_set_default_direction_recurse,
			  data);

  g_object_unref (widget);
}

/**
 * btk_widget_set_default_direction:
 * @dir: the new default direction. This cannot be
 *        %BTK_TEXT_DIR_NONE.
 * 
 * Sets the default reading direction for widgets where the
 * direction has not been explicitly set by btk_widget_set_direction().
 **/
void
btk_widget_set_default_direction (BtkTextDirection dir)
{
  g_return_if_fail (dir == BTK_TEXT_DIR_RTL || dir == BTK_TEXT_DIR_LTR);

  if (dir != btk_default_direction)
    {
      GList *toplevels, *tmp_list;
      BtkTextDirection old_dir = btk_default_direction;
      
      btk_default_direction = dir;

      tmp_list = toplevels = btk_window_list_toplevels ();
      g_list_foreach (toplevels, (GFunc)g_object_ref, NULL);
      
      while (tmp_list)
	{
	  btk_widget_set_default_direction_recurse (tmp_list->data,
						    GUINT_TO_POINTER (old_dir));
	  g_object_unref (tmp_list->data);
	  tmp_list = tmp_list->next;
	}

      g_list_free (toplevels);
    }
}

/**
 * btk_widget_get_default_direction:
 * 
 * Obtains the current default reading direction. See
 * btk_widget_set_default_direction().
 *
 * Return value: the current default direction. 
 **/
BtkTextDirection
btk_widget_get_default_direction (void)
{
  return btk_default_direction;
}

static void
btk_widget_dispose (GObject *object)
{
  BtkWidget *widget = BTK_WIDGET (object);

  if (widget->parent)
    btk_container_remove (BTK_CONTAINER (widget->parent), widget);
  else if (btk_widget_get_visible (widget))
    btk_widget_hide (widget);

  BTK_WIDGET_UNSET_FLAGS (widget, BTK_VISIBLE);
  if (btk_widget_get_realized (widget))
    btk_widget_unrealize (widget);
  
  G_OBJECT_CLASS (btk_widget_parent_class)->dispose (object);
}

static void
btk_widget_real_destroy (BtkObject *object)
{
  /* btk_object_destroy() will already hold a refcount on object */
  BtkWidget *widget = BTK_WIDGET (object);

  /* wipe accelerator closures (keep order) */
  g_object_set_qdata (G_OBJECT (widget), quark_accel_path, NULL);
  g_object_set_qdata (G_OBJECT (widget), quark_accel_closures, NULL);

  /* Callers of add_mnemonic_label() should disconnect on ::destroy */
  g_object_set_qdata (G_OBJECT (widget), quark_mnemonic_labels, NULL);
  
  btk_grab_remove (widget);
  
  g_object_unref (widget->style);
  widget->style = btk_widget_get_default_style ();
  g_object_ref (widget->style);

  BTK_OBJECT_CLASS (btk_widget_parent_class)->destroy (object);
}

static void
btk_widget_finalize (GObject *object)
{
  BtkWidget *widget = BTK_WIDGET (object);
  BtkWidgetAuxInfo *aux_info;
  BtkAccessible *accessible;
  
  btk_grab_remove (widget);

  g_object_unref (widget->style);
  widget->style = NULL;

  g_free (widget->name);
  
  aux_info =_btk_widget_get_aux_info (widget, FALSE);
  if (aux_info)
    btk_widget_aux_info_destroy (aux_info);

  accessible = g_object_get_qdata (G_OBJECT (widget), quark_accessible_object);
  if (accessible)
    g_object_unref (accessible);

  G_OBJECT_CLASS (btk_widget_parent_class)->finalize (object);
}

/*****************************************
 * btk_widget_real_map:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
btk_widget_real_map (BtkWidget *widget)
{
  g_assert (btk_widget_get_realized (widget));
  
  if (!btk_widget_get_mapped (widget))
    {
      btk_widget_set_mapped (widget, TRUE);
      
      if (btk_widget_get_has_window (widget))
	bdk_window_show (widget->window);
    }
}

/*****************************************
 * btk_widget_real_unmap:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
btk_widget_real_unmap (BtkWidget *widget)
{
  if (btk_widget_get_mapped (widget))
    {
      btk_widget_set_mapped (widget, FALSE);

      if (btk_widget_get_has_window (widget))
	bdk_window_hide (widget->window);
    }
}

/*****************************************
 * btk_widget_real_realize:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
btk_widget_real_realize (BtkWidget *widget)
{
  g_assert (!btk_widget_get_has_window (widget));
  
  btk_widget_set_realized (widget, TRUE);
  if (widget->parent)
    {
      widget->window = btk_widget_get_parent_window (widget);
      g_object_ref (widget->window);
    }
  widget->style = btk_style_attach (widget->style, widget->window);
}

/*****************************************
 * btk_widget_real_unrealize:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
btk_widget_real_unrealize (BtkWidget *widget)
{
  if (btk_widget_get_mapped (widget))
    btk_widget_real_unmap (widget);

  btk_widget_set_mapped (widget, FALSE);

  /* printf ("unrealizing %s\n", g_type_name (G_TYPE_FROM_INSTANCE (widget)));
   */

   /* We must do unrealize child widget BEFORE container widget.
    * bdk_window_destroy() destroys specified xwindow and its sub-xwindows.
    * So, unrealizing container widget bofore its children causes the problem 
    * (for example, bdk_ic_destroy () with destroyed window causes crash. )
    */

  if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget),
			  (BtkCallback) btk_widget_unrealize,
			  NULL);

  btk_style_detach (widget->style);
  if (btk_widget_get_has_window (widget))
    {
      bdk_window_set_user_data (widget->window, NULL);
      bdk_window_destroy (widget->window);
      widget->window = NULL;
    }
  else
    {
      g_object_unref (widget->window);
      widget->window = NULL;
    }

  btk_selection_remove_all (widget);
  
  btk_widget_set_realized (widget, FALSE);
}

static void
btk_widget_real_size_request (BtkWidget         *widget,
			      BtkRequisition    *requisition)
{
  requisition->width = widget->requisition.width;
  requisition->height = widget->requisition.height;
}

/**
 * _btk_widget_peek_colormap:
 * 
 * Returns colormap currently pushed by btk_widget_push_colormap, if any.
 * 
 * Return value: the currently pushed colormap, or %NULL if there is none.
 **/
BdkColormap*
_btk_widget_peek_colormap (void)
{
  if (colormap_stack)
    return (BdkColormap*) colormap_stack->data;
  return NULL;
}

/*
 * _btk_widget_set_pointer_window:
 * @widget: a #BtkWidget.
 * @pointer_window: the new pointer window.
 *
 * Sets pointer window for @widget.  Does not ref @pointer_window.
 * Actually stores it on the #BdkScreen, but you don't need to know that.
 */
void
_btk_widget_set_pointer_window (BtkWidget *widget,
                                BdkWindow *pointer_window)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  if (btk_widget_get_realized (widget))
    {
      BdkScreen *screen = bdk_window_get_screen (widget->window);

      g_object_set_qdata (G_OBJECT (screen), quark_pointer_window,
                          pointer_window);
    }
}

/*
 * _btk_widget_get_pointer_window:
 * @widget: a #BtkWidget.
 *
 * Return value: the pointer window set on the #BdkScreen @widget is attached
 * to, or %NULL.
 */
BdkWindow *
_btk_widget_get_pointer_window (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  if (btk_widget_get_realized (widget))
    {
      BdkScreen *screen = bdk_window_get_screen (widget->window);

      return g_object_get_qdata (G_OBJECT (screen), quark_pointer_window);
    }

  return NULL;
}

static void
synth_crossing (BtkWidget      *widget,
		BdkEventType    type,
		BdkWindow      *window,
		BdkCrossingMode mode,
		BdkNotifyType   detail)
{
  BdkEvent *event;
  
  event = bdk_event_new (type);

  event->crossing.window = g_object_ref (window);
  event->crossing.send_event = TRUE;
  event->crossing.subwindow = g_object_ref (window);
  event->crossing.time = BDK_CURRENT_TIME;
  event->crossing.x = event->crossing.y = 0;
  event->crossing.x_root = event->crossing.y_root = 0;
  event->crossing.mode = mode;
  event->crossing.detail = detail;
  event->crossing.focus = FALSE;
  event->crossing.state = 0;

  if (!widget)
    widget = btk_get_event_widget (event);

  if (widget)
    btk_widget_event_internal (widget, event);

  bdk_event_free (event);
}

/*
 * _btk_widget_is_pointer_widget:
 * @widget: a #BtkWidget
 *
 * Returns %TRUE if the pointer window belongs to @widget.
 */
gboolean
_btk_widget_is_pointer_widget (BtkWidget *widget)
{
  if (BTK_WIDGET_HAS_POINTER (widget))
    {
      BdkWindow *win;
      BtkWidget *wid;

      win = _btk_widget_get_pointer_window (widget);
      if (win)
        {
          bdk_window_get_user_data (win, (gpointer *)&wid);
          if (wid == widget)
            return TRUE;
        }
    }

  return FALSE;
}

/*
 * _btk_widget_synthesize_crossing:
 * @from: the #BtkWidget the virtual pointer is leaving.
 * @to: the #BtkWidget the virtual pointer is moving to.
 * @mode: the #BdkCrossingMode to place on the synthesized events.
 *
 * Generate crossing event(s) on widget state (sensitivity) or BTK+ grab change.
 *
 * The real pointer window is the window that most recently received an enter notify
 * event.  Windows that don't select for crossing events can't become the real
 * poiner window.  The real pointer widget that owns the real pointer window.  The
 * effective pointer window is the same as the real pointer window unless the real
 * pointer widget is either insensitive or there is a grab on a widget that is not
 * an ancestor of the real pointer widget (in which case the effective pointer
 * window should be the root window).
 *
 * When the effective pointer window is the same as the real poiner window, we
 * receive crossing events from the windowing system.  When the effective pointer
 * window changes to become different from the real pointer window we synthesize
 * crossing events, attempting to follow X protocol rules:
 *
 * When the root window becomes the effective pointer window:
 *   - leave notify on real pointer window, detail Ancestor
 *   - leave notify on all of its ancestors, detail Virtual
 *   - enter notify on root window, detail Inferior
 *
 * When the root window ceases to be the effective pointer window:
 *   - leave notify on root window, detail Inferior
 *   - enter notify on all ancestors of real pointer window, detail Virtual
 *   - enter notify on real pointer window, detail Ancestor
 */
void
_btk_widget_synthesize_crossing (BtkWidget      *from,
				 BtkWidget      *to,
				 BdkCrossingMode mode)
{
  BdkWindow *from_window = NULL, *to_window = NULL;

  g_return_if_fail (from != NULL || to != NULL);

  if (from != NULL)
    from_window = BTK_WIDGET_HAS_POINTER (from)
      ? _btk_widget_get_pointer_window (from) : from->window;
  if (to != NULL)
    to_window = BTK_WIDGET_HAS_POINTER (to)
      ? _btk_widget_get_pointer_window (to) : to->window;

  if (from_window == NULL && to_window == NULL)
    ;
  else if (from_window != NULL && to_window == NULL)
    {
      GList *from_ancestors = NULL, *list;
      BdkWindow *from_ancestor = from_window;

      while (from_ancestor != NULL)
	{
	  from_ancestor = bdk_window_get_effective_parent (from_ancestor);
          if (from_ancestor == NULL)
            break;
          from_ancestors = g_list_prepend (from_ancestors, from_ancestor);
	}

      synth_crossing (from, BDK_LEAVE_NOTIFY, from_window,
		      mode, BDK_NOTIFY_ANCESTOR);
      for (list = g_list_last (from_ancestors); list; list = list->prev)
	{
	  synth_crossing (NULL, BDK_LEAVE_NOTIFY, (BdkWindow *) list->data,
			  mode, BDK_NOTIFY_VIRTUAL);
	}

      /* XXX: enter/inferior on root window? */

      g_list_free (from_ancestors);
    }
  else if (from_window == NULL && to_window != NULL)
    {
      GList *to_ancestors = NULL, *list;
      BdkWindow *to_ancestor = to_window;

      while (to_ancestor != NULL)
	{
	  to_ancestor = bdk_window_get_effective_parent (to_ancestor);
	  if (to_ancestor == NULL)
            break;
          to_ancestors = g_list_prepend (to_ancestors, to_ancestor);
        }

      /* XXX: leave/inferior on root window? */

      for (list = to_ancestors; list; list = list->next)
	{
	  synth_crossing (NULL, BDK_ENTER_NOTIFY, (BdkWindow *) list->data,
			  mode, BDK_NOTIFY_VIRTUAL);
	}
      synth_crossing (to, BDK_ENTER_NOTIFY, to_window,
		      mode, BDK_NOTIFY_ANCESTOR);

      g_list_free (to_ancestors);
    }
  else if (from_window == to_window)
    ;
  else
    {
      GList *from_ancestors = NULL, *to_ancestors = NULL, *list;
      BdkWindow *from_ancestor = from_window, *to_ancestor = to_window;

      while (from_ancestor != NULL || to_ancestor != NULL)
	{
	  if (from_ancestor != NULL)
	    {
	      from_ancestor = bdk_window_get_effective_parent (from_ancestor);
	      if (from_ancestor == to_window)
		break;
              if (from_ancestor)
	        from_ancestors = g_list_prepend (from_ancestors, from_ancestor);
	    }
	  if (to_ancestor != NULL)
	    {
	      to_ancestor = bdk_window_get_effective_parent (to_ancestor);
	      if (to_ancestor == from_window)
		break;
              if (to_ancestor)
	        to_ancestors = g_list_prepend (to_ancestors, to_ancestor);
	    }
	}
      if (to_ancestor == from_window)
	{
	  if (mode != BDK_CROSSING_BTK_UNGRAB)
	    synth_crossing (from, BDK_LEAVE_NOTIFY, from_window,
			    mode, BDK_NOTIFY_INFERIOR);
	  for (list = to_ancestors; list; list = list->next)
	    synth_crossing (NULL, BDK_ENTER_NOTIFY, (BdkWindow *) list->data, 
			    mode, BDK_NOTIFY_VIRTUAL);
	  synth_crossing (to, BDK_ENTER_NOTIFY, to_window,
			  mode, BDK_NOTIFY_ANCESTOR);
	}
      else if (from_ancestor == to_window)
	{
	  synth_crossing (from, BDK_LEAVE_NOTIFY, from_window,
			  mode, BDK_NOTIFY_ANCESTOR);
	  for (list = g_list_last (from_ancestors); list; list = list->prev)
	    {
	      synth_crossing (NULL, BDK_LEAVE_NOTIFY, (BdkWindow *) list->data,
			      mode, BDK_NOTIFY_VIRTUAL);
	    }
	  if (mode != BDK_CROSSING_BTK_GRAB)
	    synth_crossing (to, BDK_ENTER_NOTIFY, to_window,
			    mode, BDK_NOTIFY_INFERIOR);
	}
      else
	{
	  while (from_ancestors != NULL && to_ancestors != NULL 
		 && from_ancestors->data == to_ancestors->data)
	    {
	      from_ancestors = g_list_delete_link (from_ancestors, 
						   from_ancestors);
	      to_ancestors = g_list_delete_link (to_ancestors, to_ancestors);
	    }

	  synth_crossing (from, BDK_LEAVE_NOTIFY, from_window,
			  mode, BDK_NOTIFY_NONLINEAR);

	  for (list = g_list_last (from_ancestors); list; list = list->prev)
	    {
	      synth_crossing (NULL, BDK_LEAVE_NOTIFY, (BdkWindow *) list->data,
			      mode, BDK_NOTIFY_NONLINEAR_VIRTUAL);
	    }
	  for (list = to_ancestors; list; list = list->next)
	    {
	      synth_crossing (NULL, BDK_ENTER_NOTIFY, (BdkWindow *) list->data,
			      mode, BDK_NOTIFY_NONLINEAR_VIRTUAL);
	    }
	  synth_crossing (to, BDK_ENTER_NOTIFY, to_window,
			  mode, BDK_NOTIFY_NONLINEAR);
	}
      g_list_free (from_ancestors);
      g_list_free (to_ancestors);
    }
}

static void
btk_widget_propagate_state (BtkWidget           *widget,
			    BtkStateData        *data)
{
  guint8 old_state = btk_widget_get_state (widget);
  guint8 old_saved_state = widget->saved_state;

  /* don't call this function with state==BTK_STATE_INSENSITIVE,
   * parent_sensitive==TRUE on a sensitive widget
   */


  if (data->parent_sensitive)
    BTK_OBJECT_FLAGS (widget) |= BTK_PARENT_SENSITIVE;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_PARENT_SENSITIVE);

  if (btk_widget_is_sensitive (widget))
    {
      if (data->state_restoration)
        widget->state = widget->saved_state;
      else
        widget->state = data->state;
    }
  else
    {
      if (!data->state_restoration)
	{
	  if (data->state != BTK_STATE_INSENSITIVE)
	    widget->saved_state = data->state;
	}
      else if (btk_widget_get_state (widget) != BTK_STATE_INSENSITIVE)
	widget->saved_state = btk_widget_get_state (widget);
      widget->state = BTK_STATE_INSENSITIVE;
    }

  if (btk_widget_is_focus (widget) && !btk_widget_is_sensitive (widget))
    {
      BtkWidget *window;

      window = btk_widget_get_toplevel (widget);
      if (window && btk_widget_is_toplevel (window))
	btk_window_set_focus (BTK_WINDOW (window), NULL);
    }

  if (old_state != btk_widget_get_state (widget) ||
      old_saved_state != widget->saved_state)
    {
      g_object_ref (widget);

      if (!btk_widget_is_sensitive (widget) && btk_widget_has_grab (widget))
	btk_grab_remove (widget);

      g_signal_emit (widget, widget_signals[STATE_CHANGED], 0, old_state);

      if (BTK_WIDGET_HAS_POINTER (widget) && !BTK_WIDGET_SHADOWED (widget))
	{
	  if (!btk_widget_is_sensitive (widget))
	    _btk_widget_synthesize_crossing (widget, NULL, 
					     BDK_CROSSING_STATE_CHANGED);
	  else if (old_state == BTK_STATE_INSENSITIVE)
	    _btk_widget_synthesize_crossing (NULL, widget, 
					     BDK_CROSSING_STATE_CHANGED);
	}

      if (BTK_IS_CONTAINER (widget))
	{
	  data->parent_sensitive = (btk_widget_is_sensitive (widget) != FALSE);
	  if (data->use_forall)
	    btk_container_forall (BTK_CONTAINER (widget),
				  (BtkCallback) btk_widget_propagate_state,
				  data);
	  else
	    btk_container_foreach (BTK_CONTAINER (widget),
				   (BtkCallback) btk_widget_propagate_state,
				   data);
	}
      g_object_unref (widget);
    }
}

/*
 * _btk_widget_get_aux_info:
 * @widget: a #BtkWidget
 * @create: if %TRUE, create the structure if it doesn't exist
 * 
 * Get the #BtkWidgetAuxInfo structure for the widget.
 * 
 * Return value: the #BtkAuxInfo structure for the widget, or
 *    %NULL if @create is %FALSE and one doesn't already exist.
 */
BtkWidgetAuxInfo*
_btk_widget_get_aux_info (BtkWidget *widget,
			  gboolean   create)
{
  BtkWidgetAuxInfo *aux_info;
  
  aux_info = g_object_get_qdata (G_OBJECT (widget), quark_aux_info);
  if (!aux_info && create)
    {
      aux_info = g_slice_new (BtkWidgetAuxInfo);

      aux_info->width = -1;
      aux_info->height = -1;
      aux_info->x = 0;
      aux_info->y = 0;
      aux_info->x_set = FALSE;
      aux_info->y_set = FALSE;
      g_object_set_qdata (G_OBJECT (widget), quark_aux_info, aux_info);
    }
  
  return aux_info;
}

/*****************************************
 * btk_widget_aux_info_destroy:
 *
 *   arguments:
 *
 *   results:
 *****************************************/

static void
btk_widget_aux_info_destroy (BtkWidgetAuxInfo *aux_info)
{
  g_slice_free (BtkWidgetAuxInfo, aux_info);
}

static void
btk_widget_shape_info_destroy (BtkWidgetShapeInfo *info)
{
  g_object_unref (info->shape_mask);
  g_slice_free (BtkWidgetShapeInfo, info);
}

/**
 * btk_widget_shape_combine_mask: 
 * @widget: a #BtkWidget
 * @shape_mask: (allow-none): shape to be added, or %NULL to remove an existing shape
 * @offset_x: X position of shape mask with respect to @window
 * @offset_y: Y position of shape mask with respect to @window
 * 
 * Sets a shape for this widget's BDK window. This allows for
 * transparent windows etc., see bdk_window_shape_combine_mask()
 * for more information.
 **/
void
btk_widget_shape_combine_mask (BtkWidget *widget,
			       BdkBitmap *shape_mask,
			       gint	  offset_x,
			       gint	  offset_y)
{
  BtkWidgetShapeInfo* shape_info;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  /*  set_shape doesn't work on widgets without bdk window */
  g_return_if_fail (btk_widget_get_has_window (widget));

  if (!shape_mask)
    {
      BTK_PRIVATE_UNSET_FLAG (widget, BTK_HAS_SHAPE_MASK);
      
      if (widget->window)
	bdk_window_shape_combine_mask (widget->window, NULL, 0, 0);
      
      g_object_set_qdata (G_OBJECT (widget), quark_shape_info, NULL);
    }
  else
    {
      BTK_PRIVATE_SET_FLAG (widget, BTK_HAS_SHAPE_MASK);
      
      shape_info = g_slice_new (BtkWidgetShapeInfo);
      g_object_set_qdata_full (G_OBJECT (widget), quark_shape_info, shape_info,
			       (GDestroyNotify) btk_widget_shape_info_destroy);
      
      shape_info->shape_mask = g_object_ref (shape_mask);
      shape_info->offset_x = offset_x;
      shape_info->offset_y = offset_y;
      
      /* set shape if widget has a bdk window already.
       * otherwise the shape is scheduled to be set by btk_widget_realize().
       */
      if (widget->window)
	bdk_window_shape_combine_mask (widget->window, shape_mask,
				       offset_x, offset_y);
    }
}

/**
 * btk_widget_input_shape_combine_mask:
 * @widget: a #BtkWidget
 * @shape_mask: (allow-none): shape to be added, or %NULL to remove an existing shape
 * @offset_x: X position of shape mask with respect to @window
 * @offset_y: Y position of shape mask with respect to @window
 *
 * Sets an input shape for this widget's BDK window. This allows for
 * windows which react to mouse click in a nonrectangular rebunnyion, see 
 * bdk_window_input_shape_combine_mask() for more information.
 *
 * Since: 2.10
 **/
void
btk_widget_input_shape_combine_mask (BtkWidget *widget,
				     BdkBitmap *shape_mask,
				     gint       offset_x,
				     gint	offset_y)
{
  BtkWidgetShapeInfo* shape_info;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  /*  set_shape doesn't work on widgets without bdk window */
  g_return_if_fail (btk_widget_get_has_window (widget));

  if (!shape_mask)
    {
      if (widget->window)
	bdk_window_input_shape_combine_mask (widget->window, NULL, 0, 0);
      
      g_object_set_qdata (G_OBJECT (widget), quark_input_shape_info, NULL);
    }
  else
    {
      shape_info = g_slice_new (BtkWidgetShapeInfo);
      g_object_set_qdata_full (G_OBJECT (widget), quark_input_shape_info, 
			       shape_info,
			       (GDestroyNotify) btk_widget_shape_info_destroy);
      
      shape_info->shape_mask = g_object_ref (shape_mask);
      shape_info->offset_x = offset_x;
      shape_info->offset_y = offset_y;
      
      /* set shape if widget has a bdk window already.
       * otherwise the shape is scheduled to be set by btk_widget_realize().
       */
      if (widget->window)
	bdk_window_input_shape_combine_mask (widget->window, shape_mask,
					     offset_x, offset_y);
    }
}


static void
btk_reset_shapes_recurse (BtkWidget *widget,
			  BdkWindow *window)
{
  gpointer data;
  GList *list;

  bdk_window_get_user_data (window, &data);
  if (data != widget)
    return;

  bdk_window_shape_combine_mask (window, NULL, 0, 0);
  for (list = bdk_window_peek_children (window); list; list = list->next)
    btk_reset_shapes_recurse (widget, list->data);
}

/**
 * btk_widget_reset_shapes:
 * @widget: a #BtkWidget
 *
 * Recursively resets the shape on this widget and its descendants.
 *
 * Deprecated: This function is being removed in BTK+ 3.0. Don't use it.
 **/
void
btk_widget_reset_shapes (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (btk_widget_get_realized (widget));

  if (!BTK_WIDGET_HAS_SHAPE_MASK (widget))
    btk_reset_shapes_recurse (widget, widget->window);
}

/**
 * btk_widget_ref:
 * @widget: a #BtkWidget
 * 
 * Adds a reference to a widget. This function is exactly the same
 * as calling g_object_ref(), and exists mostly for historical
 * reasons. It can still be convenient to avoid casting a widget
 * to a #GObject, it saves a small amount of typing.
 * 
 * Return value: the widget that was referenced
 *
 * Deprecated: 2.12: Use g_object_ref() instead.
 **/
BtkWidget*
btk_widget_ref (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  return (BtkWidget*) g_object_ref ((GObject*) widget);
}

/**
 * btk_widget_unref:
 * @widget: a #BtkWidget
 *
 * Inverse of btk_widget_ref(). Equivalent to g_object_unref().
 * 
 * Deprecated: 2.12: Use g_object_unref() instead.
 **/
void
btk_widget_unref (BtkWidget *widget)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_object_unref ((GObject*) widget);
}

static void
expose_window (BdkWindow *window)
{
  BdkEvent event;
  GList *l, *children;
  gpointer user_data;
  gboolean is_double_buffered;

  bdk_window_get_user_data (window, &user_data);

  if (user_data)
    is_double_buffered = btk_widget_get_double_buffered (BTK_WIDGET (user_data));
  else
    is_double_buffered = FALSE;
  
  event.expose.type = BDK_EXPOSE;
  event.expose.window = g_object_ref (window);
  event.expose.send_event = FALSE;
  event.expose.count = 0;
  event.expose.area.x = 0;
  event.expose.area.y = 0;
  event.expose.area.width = bdk_window_get_width (window);
  event.expose.area.height = bdk_window_get_height (window);
  event.expose.rebunnyion = bdk_rebunnyion_rectangle (&event.expose.area);

  /* If this is not double buffered, force a double buffer so that
     redirection works. */
  if (!is_double_buffered)
    bdk_window_begin_paint_rebunnyion (window, event.expose.rebunnyion);
  
  btk_main_do_event (&event);

  if (!is_double_buffered)
    bdk_window_end_paint (window);
  
  children = bdk_window_peek_children (window);
  for (l = children; l != NULL; l = l->next)
    {
      BdkWindow *child = l->data;

      /* Don't expose input-only windows */
      if (bdk_drawable_get_depth (BDK_DRAWABLE (child)) != 0)
	expose_window (l->data);
    }
  
  g_object_unref (window);
}

/**
 * btk_widget_get_snapshot:
 * @widget:    a #BtkWidget
 * @clip_rect: (allow-none): a #BdkRectangle or %NULL
 *
 * Create a #BdkPixmap of the contents of the widget and its children.
 *
 * Works even if the widget is obscured. The depth and visual of the
 * resulting pixmap is dependent on the widget being snapshot and likely
 * differs from those of a target widget displaying the pixmap.
 * The function bdk_pixbuf_get_from_drawable() can be used to convert
 * the pixmap to a visual independant representation.
 *
 * The snapshot area used by this function is the @widget's allocation plus
 * any extra space occupied by additional windows belonging to this widget
 * (such as the arrows of a spin button).
 * Thus, the resulting snapshot pixmap is possibly larger than the allocation.
 * 
 * If @clip_rect is non-%NULL, the resulting pixmap is shrunken to
 * match the specified clip_rect. The (x,y) coordinates of @clip_rect are
 * interpreted widget relative. If width or height of @clip_rect are 0 or
 * negative, the width or height of the resulting pixmap will be shrunken
 * by the respective amount.
 * For instance a @clip_rect <literal>{ +5, +5, -10, -10 }</literal> will
 * chop off 5 pixels at each side of the snapshot pixmap.
 * If non-%NULL, @clip_rect will contain the exact widget-relative snapshot
 * coordinates upon return. A @clip_rect of <literal>{ -1, -1, 0, 0 }</literal>
 * can be used to preserve the auto-grown snapshot area and use @clip_rect
 * as a pure output parameter.
 *
 * The returned pixmap can be %NULL, if the resulting @clip_area was empty.
 *
 * Return value: #BdkPixmap snapshot of the widget
 * 
 * Since: 2.14
 **/
BdkPixmap*
btk_widget_get_snapshot (BtkWidget    *widget,
                         BdkRectangle *clip_rect)
{
  int x, y, width, height;
  BdkWindow *parent_window = NULL;
  BdkPixmap *pixmap;
  GList *windows = NULL, *list;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  if (!btk_widget_get_visible (widget))
    return NULL;

  /* the widget (and parent_window) must be realized to be drawable */
  if (widget->parent && !btk_widget_get_realized (widget->parent))
    btk_widget_realize (widget->parent);
  if (!btk_widget_get_realized (widget))
    btk_widget_realize (widget);

  /* determine snapshot rectangle */
  x = widget->allocation.x;
  y = widget->allocation.y;
  width = widget->allocation.width;
  height = widget->allocation.height;

  if (widget->parent && btk_widget_get_has_window (widget))
    {
      /* grow snapshot rectangle to cover all widget windows */
      parent_window = btk_widget_get_parent_window (widget);
      for (list = bdk_window_peek_children (parent_window); list; list = list->next)
        {
          BdkWindow *subwin = list->data;
          gpointer windata;
          int wx, wy, ww, wh;
          bdk_window_get_user_data (subwin, &windata);
          if (windata != widget)
            continue;
          windows = g_list_prepend (windows, subwin);
          bdk_window_get_position (subwin, &wx, &wy);
          ww = bdk_window_get_width (subwin);
          wh = bdk_window_get_height (subwin);
          /* grow snapshot rectangle by extra widget sub window */
          if (wx < x)
            {
              width += x - wx;
              x = wx;
            }
          if (wy < y)
            {
              height += y - wy;
              y = wy;
            }
          if (x + width < wx + ww)
            width += wx + ww - (x + width);
          if (y + height < wy + wh)
            height += wy + wh - (y + height);
        }
    }
  else if (!widget->parent)
    x = y = 0; /* toplevel */

  /* at this point, (x,y,width,height) is the parent_window relative
   * snapshot area covering all of widget's windows.
   */

  /* shrink snapshot size by clip_rectangle */
  if (clip_rect)
    {
      BdkRectangle snap = { x, y, width, height }, clip = *clip_rect;
      clip.x = clip.x < 0 ? x : clip.x;
      clip.y = clip.y < 0 ? y : clip.y;
      clip.width = clip.width <= 0 ? MAX (0, width + clip.width) : clip.width;
      clip.height = clip.height <= 0 ? MAX (0, height + clip.height) : clip.height;
      if (widget->parent)
        {
          /* offset clip_rect, so it's parent_window relative */
          if (clip_rect->x >= 0)
            clip.x += widget->allocation.x;
          if (clip_rect->y >= 0)
            clip.y += widget->allocation.y;
        }
      if (!bdk_rectangle_intersect (&snap, &clip, &snap))
        {
          g_list_free (windows);
          clip_rect->width = clip_rect->height = 0;
          return NULL; /* empty snapshot area */
        }
      x = snap.x;
      y = snap.y;
      width = snap.width;
      height = snap.height;
    }

  /* render snapshot */
  pixmap = bdk_pixmap_new (widget->window, width, height, bdk_drawable_get_depth (widget->window));
  for (list = windows; list; list = list->next) /* !NO_WINDOW widgets */
    {
      BdkWindow *subwin = list->data;
      int wx, wy;
      if (bdk_drawable_get_depth (BDK_DRAWABLE (subwin)) == 0)
	continue; /* Input only window */
      bdk_window_get_position (subwin, &wx, &wy);
      bdk_window_redirect_to_drawable (subwin, pixmap, MAX (0, x - wx), MAX (0, y - wy),
                                       MAX (0, wx - x), MAX (0, wy - y), width, height);

      expose_window (subwin);
    }
  if (!windows) /* NO_WINDOW || toplevel => parent_window == NULL || parent_window == widget->window */
    {
      bdk_window_redirect_to_drawable (widget->window, pixmap, x, y, 0, 0, width, height);
      expose_window (widget->window);
    }
  for (list = windows; list; list = list->next)
    bdk_window_remove_redirection (list->data);
  if (!windows) /* NO_WINDOW || toplevel */
    bdk_window_remove_redirection (widget->window);
  g_list_free (windows);

  /* return pixmap and snapshot rectangle coordinates */
  if (clip_rect)
    {
      clip_rect->x = x;
      clip_rect->y = y;
      clip_rect->width = width;
      clip_rect->height = height;
      if (widget->parent)
        {
          /* offset clip_rect from parent_window so it's widget relative */
          clip_rect->x -= widget->allocation.x;
          clip_rect->y -= widget->allocation.y;
        }
      if (0)
        g_printerr ("btk_widget_get_snapshot: %s (%d,%d, %dx%d)\n",
                    G_OBJECT_TYPE_NAME (widget),
                    clip_rect->x, clip_rect->y, clip_rect->width, clip_rect->height);
    }
  return pixmap;
}

/* style properties
 */

/**
 * btk_widget_class_install_style_property_parser:
 * @klass: a #BtkWidgetClass
 * @pspec: the #GParamSpec for the style property
 * @parser: the parser for the style property
 * 
 * Installs a style property on a widget class. 
 **/
void
btk_widget_class_install_style_property_parser (BtkWidgetClass     *klass,
						GParamSpec         *pspec,
						BtkRcPropertyParser parser)
{
  g_return_if_fail (BTK_IS_WIDGET_CLASS (klass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (pspec->flags & G_PARAM_READABLE);
  g_return_if_fail (!(pspec->flags & (G_PARAM_CONSTRUCT_ONLY | G_PARAM_CONSTRUCT)));
  
  if (g_param_spec_pool_lookup (style_property_spec_pool, pspec->name, G_OBJECT_CLASS_TYPE (klass), FALSE))
    {
      g_warning (B_STRLOC ": class `%s' already contains a style property named `%s'",
		 G_OBJECT_CLASS_NAME (klass),
		 pspec->name);
      return;
    }

  g_param_spec_ref_sink (pspec);
  g_param_spec_set_qdata (pspec, quark_property_parser, (gpointer) parser);
  g_param_spec_pool_insert (style_property_spec_pool, pspec, G_OBJECT_CLASS_TYPE (klass));
}

/**
 * btk_widget_class_install_style_property:
 * @klass: a #BtkWidgetClass
 * @pspec: the #GParamSpec for the property
 * 
 * Installs a style property on a widget class. The parser for the
 * style property is determined by the value type of @pspec.
 **/
void
btk_widget_class_install_style_property (BtkWidgetClass *klass,
					 GParamSpec     *pspec)
{
  BtkRcPropertyParser parser;

  g_return_if_fail (BTK_IS_WIDGET_CLASS (klass));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

  parser = _btk_rc_property_parser_from_type (G_PARAM_SPEC_VALUE_TYPE (pspec));

  btk_widget_class_install_style_property_parser (klass, pspec, parser);
}

/**
 * btk_widget_class_find_style_property:
 * @klass: a #BtkWidgetClass
 * @property_name: the name of the style property to find
 * @returns: (transfer none): the #GParamSpec of the style property or
 *   %NULL if @class has no style property with that name.
 *
 * Finds a style property of a widget class by name.
 *
 * Since: 2.2
 */
GParamSpec*
btk_widget_class_find_style_property (BtkWidgetClass *klass,
				      const gchar    *property_name)
{
  g_return_val_if_fail (property_name != NULL, NULL);

  return g_param_spec_pool_lookup (style_property_spec_pool,
				   property_name,
				   G_OBJECT_CLASS_TYPE (klass),
				   TRUE);
}

/**
 * btk_widget_class_list_style_properties:
 * @klass: a #BtkWidgetClass
 * @n_properties: location to return the number of style properties found
 * @returns: (array length=n_properties) (transfer container): an newly
 *       allocated array of #GParamSpec*. The array must be freed with
 *       g_free().
 *
 * Returns all style properties of a widget class.
 *
 * Since: 2.2
 */
GParamSpec**
btk_widget_class_list_style_properties (BtkWidgetClass *klass,
					guint          *n_properties)
{
  GParamSpec **pspecs;
  guint n;

  pspecs = g_param_spec_pool_list (style_property_spec_pool,
				   G_OBJECT_CLASS_TYPE (klass),
				   &n);
  if (n_properties)
    *n_properties = n;

  return pspecs;
}

/**
 * btk_widget_style_get_property:
 * @widget: a #BtkWidget
 * @property_name: the name of a style property
 * @value: location to return the property value 
 *
 * Gets the value of a style property of @widget.
 */
void
btk_widget_style_get_property (BtkWidget   *widget,
			       const gchar *property_name,
			       GValue      *value)
{
  GParamSpec *pspec;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (property_name != NULL);
  g_return_if_fail (G_IS_VALUE (value));

  g_object_ref (widget);
  pspec = g_param_spec_pool_lookup (style_property_spec_pool,
				    property_name,
				    G_OBJECT_TYPE (widget),
				    TRUE);
  if (!pspec)
    g_warning ("%s: widget class `%s' has no property named `%s'",
	       B_STRLOC,
	       G_OBJECT_TYPE_NAME (widget),
	       property_name);
  else
    {
      const GValue *peek_value;

      peek_value = _btk_style_peek_property_value (widget->style,
						   G_OBJECT_TYPE (widget),
						   pspec,
						   (BtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser));
      
      /* auto-conversion of the caller's value type
       */
      if (G_VALUE_TYPE (value) == G_PARAM_SPEC_VALUE_TYPE (pspec))
	g_value_copy (peek_value, value);
      else if (g_value_type_transformable (G_PARAM_SPEC_VALUE_TYPE (pspec), G_VALUE_TYPE (value)))
	g_value_transform (peek_value, value);
      else
	g_warning ("can't retrieve style property `%s' of type `%s' as value of type `%s'",
		   pspec->name,
		   g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec)),
		   G_VALUE_TYPE_NAME (value));
    }
  g_object_unref (widget);
}

/**
 * btk_widget_style_get_valist:
 * @widget: a #BtkWidget
 * @first_property_name: the name of the first property to get
 * @var_args: a <type>va_list</type> of pairs of property names and
 *     locations to return the property values, starting with the location
 *     for @first_property_name.
 * 
 * Non-vararg variant of btk_widget_style_get(). Used primarily by language 
 * bindings.
 */ 
void
btk_widget_style_get_valist (BtkWidget   *widget,
			     const gchar *first_property_name,
			     va_list      var_args)
{
  const gchar *name;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_object_ref (widget);

  name = first_property_name;
  while (name)
    {
      const GValue *peek_value;
      GParamSpec *pspec;
      gchar *error;

      pspec = g_param_spec_pool_lookup (style_property_spec_pool,
					name,
					G_OBJECT_TYPE (widget),
					TRUE);
      if (!pspec)
	{
	  g_warning ("%s: widget class `%s' has no property named `%s'",
		     B_STRLOC,
		     G_OBJECT_TYPE_NAME (widget),
		     name);
	  break;
	}
      /* style pspecs are always readable so we can spare that check here */

      peek_value = _btk_style_peek_property_value (widget->style,
						   G_OBJECT_TYPE (widget),
						   pspec,
						   (BtkRcPropertyParser) g_param_spec_get_qdata (pspec, quark_property_parser));
      G_VALUE_LCOPY (peek_value, var_args, 0, &error);
      if (error)
	{
	  g_warning ("%s: %s", B_STRLOC, error);
	  g_free (error);
	  break;
	}

      name = va_arg (var_args, gchar*);
    }

  g_object_unref (widget);
}

/**
 * btk_widget_style_get:
 * @widget: a #BtkWidget
 * @first_property_name: the name of the first property to get
 * @Varargs: pairs of property names and locations to 
 *   return the property values, starting with the location for 
 *   @first_property_name, terminated by %NULL.
 *
 * Gets the values of a multiple style properties of @widget.
 */
void
btk_widget_style_get (BtkWidget   *widget,
		      const gchar *first_property_name,
		      ...)
{
  va_list var_args;

  g_return_if_fail (BTK_IS_WIDGET (widget));

  va_start (var_args, first_property_name);
  btk_widget_style_get_valist (widget, first_property_name, var_args);
  va_end (var_args);
}

/**
 * btk_widget_path:
 * @widget: a #BtkWidget
 * @path_length: (out) (allow-none): location to store length of the path, or %NULL
 * @path: (out) (allow-none):  location to store allocated path string, or %NULL
 * @path_reversed: (out) (allow-none):  location to store allocated reverse path string, or %NULL
 *
 * Obtains the full path to @widget. The path is simply the name of a
 * widget and all its parents in the container hierarchy, separated by
 * periods. The name of a widget comes from
 * btk_widget_get_name(). Paths are used to apply styles to a widget
 * in btkrc configuration files. Widget names are the type of the
 * widget by default (e.g. "BtkButton") or can be set to an
 * application-specific value with btk_widget_set_name(). By setting
 * the name of a widget, you allow users or theme authors to apply
 * styles to that specific widget in their btkrc
 * file. @path_reversed_p fills in the path in reverse order,
 * i.e. starting with @widget's name instead of starting with the name
 * of @widget's outermost ancestor.
 **/
void
btk_widget_path (BtkWidget *widget,
		 guint     *path_length,
		 gchar    **path,
		 gchar    **path_reversed)
{
  static gchar *rev_path = NULL;
  static guint tmp_path_len = 0;
  guint len;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  len = 0;
  do
    {
      const gchar *string;
      const gchar *s;
      gchar *d;
      guint l;
      
      string = btk_widget_get_name (widget);
      l = strlen (string);
      while (tmp_path_len <= len + l + 1)
	{
	  tmp_path_len += INIT_PATH_SIZE;
	  rev_path = g_realloc (rev_path, tmp_path_len);
	}
      s = string + l - 1;
      d = rev_path + len;
      while (s >= string)
	*(d++) = *(s--);
      len += l;
      
      widget = widget->parent;
      
      if (widget)
	rev_path[len++] = '.';
      else
	rev_path[len++] = 0;
    }
  while (widget);
  
  if (path_length)
    *path_length = len - 1;
  if (path_reversed)
    *path_reversed = g_strdup (rev_path);
  if (path)
    {
      *path = g_strdup (rev_path);
      g_strreverse (*path);
    }
}

/**
 * btk_widget_class_path:
 * @widget: a #BtkWidget
 * @path_length: (out) (allow-none): location to store the length of the class path, or %NULL
 * @path: (out) (allow-none): location to store the class path as an allocated string, or %NULL
 * @path_reversed: (out) (allow-none): location to store the reverse class path as an allocated
 *    string, or %NULL
 *
 * Same as btk_widget_path(), but always uses the name of a widget's type,
 * never uses a custom name set with btk_widget_set_name().
 * 
 **/
void
btk_widget_class_path (BtkWidget *widget,
		       guint     *path_length,
		       gchar    **path,
		       gchar    **path_reversed)
{
  static gchar *rev_path = NULL;
  static guint tmp_path_len = 0;
  guint len;
  
  g_return_if_fail (BTK_IS_WIDGET (widget));
  
  len = 0;
  do
    {
      const gchar *string;
      const gchar *s;
      gchar *d;
      guint l;
      
      string = g_type_name (G_OBJECT_TYPE (widget));
      l = strlen (string);
      while (tmp_path_len <= len + l + 1)
	{
	  tmp_path_len += INIT_PATH_SIZE;
	  rev_path = g_realloc (rev_path, tmp_path_len);
	}
      s = string + l - 1;
      d = rev_path + len;
      while (s >= string)
	*(d++) = *(s--);
      len += l;
      
      widget = widget->parent;
      
      if (widget)
	rev_path[len++] = '.';
      else
	rev_path[len++] = 0;
    }
  while (widget);
  
  if (path_length)
    *path_length = len - 1;
  if (path_reversed)
    *path_reversed = g_strdup (rev_path);
  if (path)
    {
      *path = g_strdup (rev_path);
      g_strreverse (*path);
    }
}

/**
 * btk_requisition_copy:
 * @requisition: a #BtkRequisition
 *
 * Copies a #BtkRequisition.
 *
 * Returns: a copy of @requisition
 **/
BtkRequisition *
btk_requisition_copy (const BtkRequisition *requisition)
{
  return (BtkRequisition *)g_memdup (requisition, sizeof (BtkRequisition));
}

/**
 * btk_requisition_free:
 * @requisition: a #BtkRequisition
 * 
 * Frees a #BtkRequisition.
 **/
void
btk_requisition_free (BtkRequisition *requisition)
{
  g_free (requisition);
}

GType
btk_requisition_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("BtkRequisition"),
					     (GBoxedCopyFunc) btk_requisition_copy,
					     (GBoxedFreeFunc) btk_requisition_free);

  return our_type;
}

/**
 * btk_widget_get_accessible:
 * @widget: a #BtkWidget
 *
 * Returns the accessible object that describes the widget to an
 * assistive technology. 
 * 
 * If no accessibility library is loaded (i.e. no BATK implementation library is 
 * loaded via <envar>BTK_MODULES</envar> or via another application library, 
 * such as libbunny), then this #BatkObject instance may be a no-op. Likewise, 
 * if no class-specific #BatkObject implementation is available for the widget 
 * instance in question, it will inherit an #BatkObject implementation from the 
 * first ancestor class for which such an implementation is defined.
 *
 * The documentation of the <ulink url="http://developer.bunny.org/doc/API/2.0/batk/index.html">BATK</ulink>
 * library contains more information about accessible objects and their uses.
 *
 * Returns: (transfer none): the #BatkObject associated with @widget
 */
BatkObject*
btk_widget_get_accessible (BtkWidget *widget)
{
  BtkWidgetClass *klass;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  klass = BTK_WIDGET_GET_CLASS (widget);

  g_return_val_if_fail (klass->get_accessible != NULL, NULL);

  return klass->get_accessible (widget);
}

static BatkObject* 
btk_widget_real_get_accessible (BtkWidget *widget)
{
  BatkObject* accessible;

  accessible = g_object_get_qdata (G_OBJECT (widget), 
                                   quark_accessible_object);
  if (!accessible)
  {
    BatkObjectFactory *factory;
    BatkRegistry *default_registry;

    default_registry = batk_get_default_registry ();
    factory = batk_registry_get_factory (default_registry, 
                                        G_TYPE_FROM_INSTANCE (widget));
    accessible =
      batk_object_factory_create_accessible (factory,
					    G_OBJECT (widget));
    g_object_set_qdata (G_OBJECT (widget), 
                        quark_accessible_object,
                        accessible);
  }
  return accessible;
}

/*
 * Initialize a BatkImplementorIface instance's virtual pointers as
 * appropriate to this implementor's class (BtkWidget).
 */
static void
btk_widget_accessible_interface_init (BatkImplementorIface *iface)
{
  iface->ref_accessible = btk_widget_ref_accessible;
}

static BatkObject*
btk_widget_ref_accessible (BatkImplementor *implementor)
{
  BatkObject *accessible;

  accessible = btk_widget_get_accessible (BTK_WIDGET (implementor));
  if (accessible)
    g_object_ref (accessible);
  return accessible;
}

/*
 * BtkBuildable implementation
 */
static GQuark		 quark_builder_has_default = 0;
static GQuark		 quark_builder_has_focus = 0;
static GQuark		 quark_builder_batk_relations = 0;
static GQuark            quark_builder_set_name = 0;

static void
btk_widget_buildable_interface_init (BtkBuildableIface *iface)
{
  quark_builder_has_default = g_quark_from_static_string ("btk-builder-has-default");
  quark_builder_has_focus = g_quark_from_static_string ("btk-builder-has-focus");
  quark_builder_batk_relations = g_quark_from_static_string ("btk-builder-batk-relations");
  quark_builder_set_name = g_quark_from_static_string ("btk-builder-set-name");

  iface->set_name = btk_widget_buildable_set_name;
  iface->get_name = btk_widget_buildable_get_name;
  iface->get_internal_child = btk_widget_buildable_get_internal_child;
  iface->set_buildable_property = btk_widget_buildable_set_buildable_property;
  iface->parser_finished = btk_widget_buildable_parser_finished;
  iface->custom_tag_start = btk_widget_buildable_custom_tag_start;
  iface->custom_finished = btk_widget_buildable_custom_finished;
}

static void
btk_widget_buildable_set_name (BtkBuildable *buildable,
			       const gchar  *name)
{
  g_object_set_qdata_full (G_OBJECT (buildable), quark_builder_set_name,
                           g_strdup (name), g_free);
}

static const gchar *
btk_widget_buildable_get_name (BtkBuildable *buildable)
{
  return g_object_get_qdata (G_OBJECT (buildable), quark_builder_set_name);
}

static GObject *
btk_widget_buildable_get_internal_child (BtkBuildable *buildable,
					 BtkBuilder   *builder,
					 const gchar  *childname)
{
  if (strcmp (childname, "accessible") == 0)
    return G_OBJECT (btk_widget_get_accessible (BTK_WIDGET (buildable)));

  return NULL;
}

static void
btk_widget_buildable_set_buildable_property (BtkBuildable *buildable,
					     BtkBuilder   *builder,
					     const gchar  *name,
					     const GValue *value)
{
  if (strcmp (name, "has-default") == 0 && g_value_get_boolean (value))
      g_object_set_qdata (G_OBJECT (buildable), quark_builder_has_default,
			  GINT_TO_POINTER (TRUE));
  else if (strcmp (name, "has-focus") == 0 && g_value_get_boolean (value))
      g_object_set_qdata (G_OBJECT (buildable), quark_builder_has_focus,
			  GINT_TO_POINTER (TRUE));
  else
    g_object_set_property (G_OBJECT (buildable), name, value);
}

typedef struct
{
  gchar *action_name;
  GString *description;
  gchar *context;
  gboolean translatable;
} BatkActionData;

typedef struct
{
  gchar *target;
  gchar *type;
} BatkRelationData;

static void
free_action (BatkActionData *data, gpointer user_data)
{
  g_free (data->action_name);
  g_string_free (data->description, TRUE);
  g_free (data->context);
  g_slice_free (BatkActionData, data);
}

static void
free_relation (BatkRelationData *data, gpointer user_data)
{
  g_free (data->target);
  g_free (data->type);
  g_slice_free (BatkRelationData, data);
}

static void
btk_widget_buildable_parser_finished (BtkBuildable *buildable,
				      BtkBuilder   *builder)
{
  GSList *batk_relations;

  if (g_object_get_qdata (G_OBJECT (buildable), quark_builder_has_default))
    btk_widget_grab_default (BTK_WIDGET (buildable));
  if (g_object_get_qdata (G_OBJECT (buildable), quark_builder_has_focus))
    btk_widget_grab_focus (BTK_WIDGET (buildable));

  batk_relations = g_object_get_qdata (G_OBJECT (buildable),
				      quark_builder_batk_relations);
  if (batk_relations)
    {
      BatkObject *accessible;
      BatkRelationSet *relation_set;
      GSList *l;
      GObject *target;
      BatkRelationType relation_type;
      BatkObject *target_accessible;

      accessible = btk_widget_get_accessible (BTK_WIDGET (buildable));
      relation_set = batk_object_ref_relation_set (accessible);

      for (l = batk_relations; l; l = l->next)
	{
	  BatkRelationData *relation = (BatkRelationData*)l->data;

	  target = btk_builder_get_object (builder, relation->target);
	  if (!target)
	    {
	      g_warning ("Target object %s in <relation> does not exist",
			 relation->target);
	      continue;
	    }
	  target_accessible = btk_widget_get_accessible (BTK_WIDGET (target));
	  g_assert (target_accessible != NULL);

	  relation_type = batk_relation_type_for_name (relation->type);
	  if (relation_type == BATK_RELATION_NULL)
	    {
	      g_warning ("<relation> type %s not found",
			 relation->type);
	      continue;
	    }
	  batk_relation_set_add_relation_by_type (relation_set, relation_type,
						 target_accessible);
	}
      g_object_unref (relation_set);

      g_slist_foreach (batk_relations, (GFunc)free_relation, NULL);
      g_slist_free (batk_relations);
      g_object_set_qdata (G_OBJECT (buildable), quark_builder_batk_relations,
			  NULL);
    }
}

typedef struct
{
  GSList *actions;
  GSList *relations;
} AccessibilitySubParserData;

static void
accessibility_start_element (GMarkupParseContext  *context,
			     const gchar          *element_name,
			     const gchar         **names,
			     const gchar         **values,
			     gpointer              user_data,
			     GError              **error)
{
  AccessibilitySubParserData *data = (AccessibilitySubParserData*)user_data;
  guint i;
  gint line_number, char_number;

  if (strcmp (element_name, "relation") == 0)
    {
      gchar *target = NULL;
      gchar *type = NULL;
      BatkRelationData *relation;

      for (i = 0; names[i]; i++)
	{
	  if (strcmp (names[i], "target") == 0)
	    target = g_strdup (values[i]);
	  else if (strcmp (names[i], "type") == 0)
	    type = g_strdup (values[i]);
	  else
	    {
	      g_markup_parse_context_get_position (context,
						   &line_number,
						   &char_number);
	      g_set_error (error,
			   BTK_BUILDER_ERROR,
			   BTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
			   "%s:%d:%d '%s' is not a valid attribute of <%s>",
			   "<input>",
			   line_number, char_number, names[i], "relation");
	      g_free (target);
	      g_free (type);
	      return;
	    }
	}

      if (!target || !type)
	{
	  g_markup_parse_context_get_position (context,
					       &line_number,
					       &char_number);
	  g_set_error (error,
		       BTK_BUILDER_ERROR,
		       BTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
		       "%s:%d:%d <%s> requires attribute \"%s\"",
		       "<input>",
		       line_number, char_number, "relation",
		       type ? "target" : "type");
	  g_free (target);
	  g_free (type);
	  return;
	}

      relation = g_slice_new (BatkRelationData);
      relation->target = target;
      relation->type = type;

      data->relations = g_slist_prepend (data->relations, relation);
    }
  else if (strcmp (element_name, "action") == 0)
    {
      const gchar *action_name = NULL;
      const gchar *description = NULL;
      const gchar *msg_context = NULL;
      gboolean translatable = FALSE;
      BatkActionData *action;

      for (i = 0; names[i]; i++)
	{
	  if (strcmp (names[i], "action_name") == 0)
	    action_name = values[i];
	  else if (strcmp (names[i], "description") == 0)
	    description = values[i];
          else if (strcmp (names[i], "translatable") == 0)
            {
              if (!_btk_builder_boolean_from_string (values[i], &translatable, error))
                return;
            }
          else if (strcmp (names[i], "comments") == 0)
            {
              /* do nothing, comments are for translators */
            }
          else if (strcmp (names[i], "context") == 0)
            msg_context = values[i];
	  else
	    {
	      g_markup_parse_context_get_position (context,
						   &line_number,
						   &char_number);
	      g_set_error (error,
			   BTK_BUILDER_ERROR,
			   BTK_BUILDER_ERROR_INVALID_ATTRIBUTE,
			   "%s:%d:%d '%s' is not a valid attribute of <%s>",
			   "<input>",
			   line_number, char_number, names[i], "action");
	      return;
	    }
	}

      if (!action_name)
	{
	  g_markup_parse_context_get_position (context,
					       &line_number,
					       &char_number);
	  g_set_error (error,
		       BTK_BUILDER_ERROR,
		       BTK_BUILDER_ERROR_MISSING_ATTRIBUTE,
		       "%s:%d:%d <%s> requires attribute \"%s\"",
		       "<input>",
		       line_number, char_number, "action",
		       "action_name");
	  return;
	}

      action = g_slice_new (BatkActionData);
      action->action_name = g_strdup (action_name);
      action->description = g_string_new (description);
      action->context = g_strdup (msg_context);
      action->translatable = translatable;

      data->actions = g_slist_prepend (data->actions, action);
    }
  else if (strcmp (element_name, "accessibility") == 0)
    ;
  else
    g_warning ("Unsupported tag for BtkWidget: %s\n", element_name);
}

static void
accessibility_text (GMarkupParseContext  *context,
                    const gchar          *text,
                    gsize                 text_len,
                    gpointer              user_data,
                    GError              **error)
{
  AccessibilitySubParserData *data = (AccessibilitySubParserData*)user_data;

  if (strcmp (g_markup_parse_context_get_element (context), "action") == 0)
    {
      BatkActionData *action = data->actions->data;

      g_string_append_len (action->description, text, text_len);
    }
}

static const GMarkupParser accessibility_parser =
  {
    accessibility_start_element,
    NULL,
    accessibility_text,
  };

typedef struct
{
  GObject *object;
  guint    key;
  guint    modifiers;
  gchar   *signal;
} AccelGroupParserData;

static void
accel_group_start_element (GMarkupParseContext  *context,
			   const gchar          *element_name,
			   const gchar         **names,
			   const gchar         **values,
			   gpointer              user_data,
			   GError              **error)
{
  gint i;
  guint key = 0;
  guint modifiers = 0;
  gchar *signal = NULL;
  AccelGroupParserData *parser_data = (AccelGroupParserData*)user_data;

  for (i = 0; names[i]; i++)
    {
      if (strcmp (names[i], "key") == 0)
	key = bdk_keyval_from_name (values[i]);
      else if (strcmp (names[i], "modifiers") == 0)
	{
	  if (!_btk_builder_flags_from_string (BDK_TYPE_MODIFIER_TYPE,
					       values[i],
					       &modifiers,
					       error))
	      return;
	}
      else if (strcmp (names[i], "signal") == 0)
	signal = g_strdup (values[i]);
    }

  if (key == 0 || signal == NULL)
    {
      g_warning ("<accelerator> requires key and signal attributes");
      return;
    }
  parser_data->key = key;
  parser_data->modifiers = modifiers;
  parser_data->signal = signal;
}

static const GMarkupParser accel_group_parser =
  {
    accel_group_start_element,
  };

static gboolean
btk_widget_buildable_custom_tag_start (BtkBuildable     *buildable,
				       BtkBuilder       *builder,
				       GObject          *child,
				       const gchar      *tagname,
				       GMarkupParser    *parser,
				       gpointer         *data)
{
  g_assert (buildable);

  if (strcmp (tagname, "accelerator") == 0)
    {
      AccelGroupParserData *parser_data;

      parser_data = g_slice_new0 (AccelGroupParserData);
      parser_data->object = g_object_ref (buildable);
      *parser = accel_group_parser;
      *data = parser_data;
      return TRUE;
    }
  if (strcmp (tagname, "accessibility") == 0)
    {
      AccessibilitySubParserData *parser_data;

      parser_data = g_slice_new0 (AccessibilitySubParserData);
      *parser = accessibility_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

void
_btk_widget_buildable_finish_accelerator (BtkWidget *widget,
					  BtkWidget *toplevel,
					  gpointer   user_data)
{
  AccelGroupParserData *accel_data;
  GSList *accel_groups;
  BtkAccelGroup *accel_group;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_IS_WIDGET (toplevel));
  g_return_if_fail (user_data != NULL);

  accel_data = (AccelGroupParserData*)user_data;
  accel_groups = btk_accel_groups_from_object (G_OBJECT (toplevel));
  if (g_slist_length (accel_groups) == 0)
    {
      accel_group = btk_accel_group_new ();
      btk_window_add_accel_group (BTK_WINDOW (toplevel), accel_group);
    }
  else
    {
      g_assert (g_slist_length (accel_groups) == 1);
      accel_group = g_slist_nth_data (accel_groups, 0);
    }

  btk_widget_add_accelerator (BTK_WIDGET (accel_data->object),
			      accel_data->signal,
			      accel_group,
			      accel_data->key,
			      accel_data->modifiers,
			      BTK_ACCEL_VISIBLE);

  g_object_unref (accel_data->object);
  g_free (accel_data->signal);
  g_slice_free (AccelGroupParserData, accel_data);
}

static void
btk_widget_buildable_custom_finished (BtkBuildable *buildable,
				      BtkBuilder   *builder,
				      GObject      *child,
				      const gchar  *tagname,
				      gpointer      user_data)
{
  AccelGroupParserData *accel_data;
  AccessibilitySubParserData *a11y_data;
  BtkWidget *toplevel;

  if (strcmp (tagname, "accelerator") == 0)
    {
      accel_data = (AccelGroupParserData*)user_data;
      g_assert (accel_data->object);

      toplevel = btk_widget_get_toplevel (BTK_WIDGET (accel_data->object));

      _btk_widget_buildable_finish_accelerator (BTK_WIDGET (buildable), toplevel, user_data);
    }
  else if (strcmp (tagname, "accessibility") == 0)
    {
      a11y_data = (AccessibilitySubParserData*)user_data;

      if (a11y_data->actions)
	{
	  BatkObject *accessible;
	  BatkAction *action;
	  gint i, n_actions;
	  GSList *l;

	  accessible = btk_widget_get_accessible (BTK_WIDGET (buildable));

          if (BATK_IS_ACTION (accessible))
            {
	      action = BATK_ACTION (accessible);
	      n_actions = batk_action_get_n_actions (action);

	      for (l = a11y_data->actions; l; l = l->next)
	        {
	          BatkActionData *action_data = (BatkActionData*)l->data;

	          for (i = 0; i < n_actions; i++)
		    if (strcmp (batk_action_get_name (action, i),
		  	        action_data->action_name) == 0)
		      break;

	          if (i < n_actions)
                    {
                      gchar *description;

                      if (action_data->translatable && action_data->description->len)
                        description = _btk_builder_parser_translate (btk_builder_get_translation_domain (builder),
                                                                     action_data->context,
                                                                     action_data->description->str);
                      else
                        description = action_data->description->str;

		      batk_action_set_description (action, i, description);
                    }
                }
	    }
          else
            g_warning ("accessibility action on a widget that does not implement BatkAction");

	  g_slist_foreach (a11y_data->actions, (GFunc)free_action, NULL);
	  g_slist_free (a11y_data->actions);
	}

      if (a11y_data->relations)
	g_object_set_qdata (G_OBJECT (buildable), quark_builder_batk_relations,
			    a11y_data->relations);

      g_slice_free (AccessibilitySubParserData, a11y_data);
    }
}


/**
 * btk_widget_get_clipboard:
 * @widget: a #BtkWidget
 * @selection: a #BdkAtom which identifies the clipboard
 *             to use. %BDK_SELECTION_CLIPBOARD gives the
 *             default clipboard. Another common value
 *             is %BDK_SELECTION_PRIMARY, which gives
 *             the primary X selection. 
 * 
 * Returns the clipboard object for the given selection to
 * be used with @widget. @widget must have a #BdkDisplay
 * associated with it, so must be attached to a toplevel
 * window.
 *
 * Return value: (transfer none): the appropriate clipboard object. If no
 *             clipboard already exists, a new one will
 *             be created. Once a clipboard object has
 *             been created, it is persistent for all time.
 *
 * Since: 2.2
 **/
BtkClipboard *
btk_widget_get_clipboard (BtkWidget *widget, BdkAtom selection)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (btk_widget_has_screen (widget), NULL);
  
  return btk_clipboard_get_for_display (btk_widget_get_display (widget),
					selection);
}

/**
 * btk_widget_list_mnemonic_labels:
 * @widget: a #BtkWidget
 * 
 * Returns a newly allocated list of the widgets, normally labels, for 
 * which this widget is a the target of a mnemonic (see for example, 
 * btk_label_set_mnemonic_widget()).

 * The widgets in the list are not individually referenced. If you
 * want to iterate through the list and perform actions involving
 * callbacks that might destroy the widgets, you
 * <emphasis>must</emphasis> call <literal>g_list_foreach (result,
 * (GFunc)g_object_ref, NULL)</literal> first, and then unref all the
 * widgets afterwards.

 * Return value: (element-type BtkWidget) (transfer container): the list of
 *  mnemonic labels; free this list
 *  with g_list_free() when you are done with it.
 *
 * Since: 2.4
 **/
GList *
btk_widget_list_mnemonic_labels (BtkWidget *widget)
{
  GList *list = NULL;
  GSList *l;
  
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  for (l = g_object_get_qdata (G_OBJECT (widget), quark_mnemonic_labels); l; l = l->next)
    list = g_list_prepend (list, l->data);

  return list;
}

/**
 * btk_widget_add_mnemonic_label:
 * @widget: a #BtkWidget
 * @label: a #BtkWidget that acts as a mnemonic label for @widget
 * 
 * Adds a widget to the list of mnemonic labels for
 * this widget. (See btk_widget_list_mnemonic_labels()). Note the
 * list of mnemonic labels for the widget is cleared when the
 * widget is destroyed, so the caller must make sure to update
 * its internal state at this point as well, by using a connection
 * to the #BtkWidget::destroy signal or a weak notifier.
 *
 * Since: 2.4
 **/
void
btk_widget_add_mnemonic_label (BtkWidget *widget,
                               BtkWidget *label)
{
  GSList *old_list, *new_list;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_IS_WIDGET (label));

  old_list = g_object_steal_qdata (G_OBJECT (widget), quark_mnemonic_labels);
  new_list = g_slist_prepend (old_list, label);
  
  g_object_set_qdata_full (G_OBJECT (widget), quark_mnemonic_labels,
			   new_list, (GDestroyNotify) g_slist_free);
}

/**
 * btk_widget_remove_mnemonic_label:
 * @widget: a #BtkWidget
 * @label: a #BtkWidget that was previously set as a mnemnic label for
 *         @widget with btk_widget_add_mnemonic_label().
 * 
 * Removes a widget from the list of mnemonic labels for
 * this widget. (See btk_widget_list_mnemonic_labels()). The widget
 * must have previously been added to the list with
 * btk_widget_add_mnemonic_label().
 *
 * Since: 2.4
 **/
void
btk_widget_remove_mnemonic_label (BtkWidget *widget,
                                  BtkWidget *label)
{
  GSList *old_list, *new_list;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (BTK_IS_WIDGET (label));

  old_list = g_object_steal_qdata (G_OBJECT (widget), quark_mnemonic_labels);
  new_list = g_slist_remove (old_list, label);

  if (new_list)
    g_object_set_qdata_full (G_OBJECT (widget), quark_mnemonic_labels,
			     new_list, (GDestroyNotify) g_slist_free);
}

/**
 * btk_widget_get_no_show_all:
 * @widget: a #BtkWidget
 * 
 * Returns the current value of the BtkWidget:no-show-all property, 
 * which determines whether calls to btk_widget_show_all() and 
 * btk_widget_hide_all() will affect this widget. 
 * 
 * Return value: the current value of the "no-show-all" property.
 *
 * Since: 2.4
 **/
gboolean
btk_widget_get_no_show_all (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  
  return (BTK_OBJECT_FLAGS (widget) & BTK_NO_SHOW_ALL) != 0;
}

/**
 * btk_widget_set_no_show_all:
 * @widget: a #BtkWidget
 * @no_show_all: the new value for the "no-show-all" property
 * 
 * Sets the #BtkWidget:no-show-all property, which determines whether 
 * calls to btk_widget_show_all() and btk_widget_hide_all() will affect 
 * this widget. 
 *
 * This is mostly for use in constructing widget hierarchies with externally
 * controlled visibility, see #BtkUIManager.
 * 
 * Since: 2.4
 **/
void
btk_widget_set_no_show_all (BtkWidget *widget,
			    gboolean   no_show_all)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  no_show_all = (no_show_all != FALSE);

  if (no_show_all == btk_widget_get_no_show_all (widget))
    return;

  if (no_show_all)
    BTK_OBJECT_FLAGS (widget) |= BTK_NO_SHOW_ALL;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_NO_SHOW_ALL);
  
  g_object_notify (G_OBJECT (widget), "no-show-all");
}


static void
btk_widget_real_set_has_tooltip (BtkWidget *widget,
			         gboolean   has_tooltip,
			         gboolean   force)
{
  gboolean priv_has_tooltip;

  priv_has_tooltip = GPOINTER_TO_UINT (g_object_get_qdata (G_OBJECT (widget),
				       quark_has_tooltip));

  if (priv_has_tooltip != has_tooltip || force)
    {
      priv_has_tooltip = has_tooltip;

      if (priv_has_tooltip)
        {
	  if (btk_widget_get_realized (widget) && !btk_widget_get_has_window (widget))
	    bdk_window_set_events (widget->window,
				   bdk_window_get_events (widget->window) |
				   BDK_LEAVE_NOTIFY_MASK |
				   BDK_POINTER_MOTION_MASK |
				   BDK_POINTER_MOTION_HINT_MASK);

	  if (btk_widget_get_has_window (widget))
	      btk_widget_add_events (widget,
				     BDK_LEAVE_NOTIFY_MASK |
				     BDK_POINTER_MOTION_MASK |
				     BDK_POINTER_MOTION_HINT_MASK);
	}

      g_object_set_qdata (G_OBJECT (widget), quark_has_tooltip,
			  GUINT_TO_POINTER (priv_has_tooltip));
    }
}

/**
 * btk_widget_set_tooltip_window:
 * @widget: a #BtkWidget
 * @custom_window: (allow-none): a #BtkWindow, or %NULL
 *
 * Replaces the default, usually yellow, window used for displaying
 * tooltips with @custom_window. BTK+ will take care of showing and
 * hiding @custom_window at the right moment, to behave likewise as
 * the default tooltip window. If @custom_window is %NULL, the default
 * tooltip window will be used.
 *
 * If the custom window should have the default theming it needs to
 * have the name "btk-tooltip", see btk_widget_set_name().
 *
 * Since: 2.12
 */
void
btk_widget_set_tooltip_window (BtkWidget *widget,
			       BtkWindow *custom_window)
{
  gboolean has_tooltip;
  gchar *tooltip_markup;

  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (custom_window == NULL || BTK_IS_WINDOW (custom_window));

  tooltip_markup = g_object_get_qdata (G_OBJECT (widget), quark_tooltip_markup);

  if (custom_window)
    g_object_ref (custom_window);

  g_object_set_qdata_full (G_OBJECT (widget), quark_tooltip_window,
			   custom_window, g_object_unref);

  has_tooltip = (custom_window != NULL || tooltip_markup != NULL);
  btk_widget_real_set_has_tooltip (widget, has_tooltip, FALSE);

  if (has_tooltip && btk_widget_get_visible (widget))
    btk_widget_queue_tooltip_query (widget);
}

/**
 * btk_widget_get_tooltip_window:
 * @widget: a #BtkWidget
 *
 * Returns the #BtkWindow of the current tooltip. This can be the
 * BtkWindow created by default, or the custom tooltip window set
 * using btk_widget_set_tooltip_window().
 *
 * Return value: (transfer none): The #BtkWindow of the current tooltip.
 *
 * Since: 2.12
 */
BtkWindow *
btk_widget_get_tooltip_window (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  return g_object_get_qdata (G_OBJECT (widget), quark_tooltip_window);
}

/**
 * btk_widget_trigger_tooltip_query:
 * @widget: a #BtkWidget
 *
 * Triggers a tooltip query on the display where the toplevel of @widget
 * is located. See btk_tooltip_trigger_tooltip_query() for more
 * information.
 *
 * Since: 2.12
 */
void
btk_widget_trigger_tooltip_query (BtkWidget *widget)
{
  btk_tooltip_trigger_tooltip_query (btk_widget_get_display (widget));
}

static guint tooltip_query_id;
static GSList *tooltip_query_displays;

static gboolean
tooltip_query_idle (gpointer data)
{
  g_slist_foreach (tooltip_query_displays, (GFunc)btk_tooltip_trigger_tooltip_query, NULL);
  g_slist_foreach (tooltip_query_displays, (GFunc)g_object_unref, NULL);
  g_slist_free (tooltip_query_displays);

  tooltip_query_displays = NULL;
  tooltip_query_id = 0;

  return FALSE;
}

static void
btk_widget_queue_tooltip_query (BtkWidget *widget)
{
  BdkDisplay *display;

  display = btk_widget_get_display (widget);

  if (!g_slist_find (tooltip_query_displays, display))
    tooltip_query_displays = g_slist_prepend (tooltip_query_displays, g_object_ref (display));

  if (tooltip_query_id == 0)
    tooltip_query_id = bdk_threads_add_idle (tooltip_query_idle, NULL);
}

/**
 * btk_widget_set_tooltip_text:
 * @widget: a #BtkWidget
 * @text: the contents of the tooltip for @widget
 *
 * Sets @text as the contents of the tooltip. This function will take
 * care of setting BtkWidget:has-tooltip to %TRUE and of the default
 * handler for the BtkWidget::query-tooltip signal.
 *
 * See also the BtkWidget:tooltip-text property and btk_tooltip_set_text().
 *
 * Since: 2.12
 */
void
btk_widget_set_tooltip_text (BtkWidget   *widget,
                             const gchar *text)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_object_set (G_OBJECT (widget), "tooltip-text", text, NULL);
}

/**
 * btk_widget_get_tooltip_text:
 * @widget: a #BtkWidget
 *
 * Gets the contents of the tooltip for @widget.
 *
 * Return value: the tooltip text, or %NULL. You should free the
 *   returned string with g_free() when done.
 *
 * Since: 2.12
 */
gchar *
btk_widget_get_tooltip_text (BtkWidget *widget)
{
  gchar *text = NULL;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  g_object_get (G_OBJECT (widget), "tooltip-text", &text, NULL);

  return text;
}

/**
 * btk_widget_set_tooltip_markup:
 * @widget: a #BtkWidget
 * @markup: (allow-none): the contents of the tooltip for @widget, or %NULL
 *
 * Sets @markup as the contents of the tooltip, which is marked up with
 *  the <link linkend="BangoMarkupFormat">Bango text markup language</link>.
 *
 * This function will take care of setting BtkWidget:has-tooltip to %TRUE
 * and of the default handler for the BtkWidget::query-tooltip signal.
 *
 * See also the BtkWidget:tooltip-markup property and
 * btk_tooltip_set_markup().
 *
 * Since: 2.12
 */
void
btk_widget_set_tooltip_markup (BtkWidget   *widget,
                               const gchar *markup)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_object_set (G_OBJECT (widget), "tooltip-markup", markup, NULL);
}

/**
 * btk_widget_get_tooltip_markup:
 * @widget: a #BtkWidget
 *
 * Gets the contents of the tooltip for @widget.
 *
 * Return value: the tooltip text, or %NULL. You should free the
 *   returned string with g_free() when done.
 *
 * Since: 2.12
 */
gchar *
btk_widget_get_tooltip_markup (BtkWidget *widget)
{
  gchar *text = NULL;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  g_object_get (G_OBJECT (widget), "tooltip-markup", &text, NULL);

  return text;
}

/**
 * btk_widget_set_has_tooltip:
 * @widget: a #BtkWidget
 * @has_tooltip: whether or not @widget has a tooltip.
 *
 * Sets the has-tooltip property on @widget to @has_tooltip.  See
 * BtkWidget:has-tooltip for more information.
 *
 * Since: 2.12
 */
void
btk_widget_set_has_tooltip (BtkWidget *widget,
			    gboolean   has_tooltip)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));

  g_object_set (G_OBJECT (widget), "has-tooltip", has_tooltip, NULL);
}

/**
 * btk_widget_get_has_tooltip:
 * @widget: a #BtkWidget
 *
 * Returns the current value of the has-tooltip property.  See
 * BtkWidget:has-tooltip for more information.
 *
 * Return value: current value of has-tooltip on @widget.
 *
 * Since: 2.12
 */
gboolean
btk_widget_get_has_tooltip (BtkWidget *widget)
{
  gboolean has_tooltip = FALSE;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);

  g_object_get (G_OBJECT (widget), "has-tooltip", &has_tooltip, NULL);

  return has_tooltip;
}

/**
 * btk_widget_get_allocation:
 * @widget: a #BtkWidget
 * @allocation: (out): a pointer to a #BtkAllocation to copy to
 *
 * Retrieves the widget's allocation.
 *
 * Since: 2.18
 */
void
btk_widget_get_allocation (BtkWidget     *widget,
                           BtkAllocation *allocation)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (allocation != NULL);

  *allocation = widget->allocation;
}

/**
 * btk_widget_set_allocation:
 * @widget: a #BtkWidget
 * @allocation: a pointer to a #BtkAllocation to copy from
 *
 * Sets the widget's allocation.  This should not be used
 * directly, but from within a widget's size_allocate method.
 *
 * Since: 2.18
 */
void
btk_widget_set_allocation (BtkWidget           *widget,
                           const BtkAllocation *allocation)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
}

/**
 * btk_widget_get_requisition:
 * @widget: a #BtkWidget
 * @requisition: (out): a pointer to a #BtkRequisition to copy to
 *
 * Retrieves the widget's requisition.
 *
 * This function should only be used by widget implementations in
 * order to figure whether the widget's requisition has actually
 * changed after some internal state change (so that they can call
 * btk_widget_queue_resize() instead of btk_widget_queue_draw()).
 *
 * Normally, btk_widget_size_request() should be used.
 *
 * Since: 2.20
 */
void
btk_widget_get_requisition (BtkWidget      *widget,
                            BtkRequisition *requisition)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (requisition != NULL);

  *requisition = widget->requisition;
}

/**
 * btk_widget_set_window:
 * @widget: a #BtkWidget
 * @window: a #BdkWindow
 *
 * Sets a widget's window. This function should only be used in a
 * widget's BtkWidget::realize() implementation. The %window passed is
 * usually either new window created with bdk_window_new(), or the
 * window of its parent widget as returned by
 * btk_widget_get_parent_window().
 *
 * Widgets must indicate whether they will create their own #BdkWindow
 * by calling btk_widget_set_has_window(). This is usually done in the
 * widget's init() function.
 *
 * Since: 2.18
 */
void
btk_widget_set_window (BtkWidget *widget,
                       BdkWindow *window)
{
  g_return_if_fail (BTK_IS_WIDGET (widget));
  g_return_if_fail (window == NULL || BDK_IS_WINDOW (window));

  if (widget->window != window)
    {
      widget->window = window;
      g_object_notify (G_OBJECT (widget), "window");
    }
}

/**
 * btk_widget_get_window:
 * @widget: a #BtkWidget
 *
 * Returns the widget's window if it is realized, %NULL otherwise
 *
 * Return value: (transfer none): @widget's window.
 *
 * Since: 2.14
 */
BdkWindow*
btk_widget_get_window (BtkWidget *widget)
{
  g_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

  return widget->window;
}

static void
_btk_widget_set_has_focus (BtkWidget *widget,
                           gboolean   has_focus)
{
  if (has_focus)
    BTK_OBJECT_FLAGS (widget) |= BTK_HAS_FOCUS;
  else
    BTK_OBJECT_FLAGS (widget) &= ~(BTK_HAS_FOCUS);
}

/**
 * btk_widget_send_focus_change:
 * @widget: a #BtkWidget
 * @event: a #BdkEvent of type BDK_FOCUS_CHANGE
 *
 * Sends the focus change @event to @widget
 *
 * This function is not meant to be used by applications. The only time it
 * should be used is when it is necessary for a #BtkWidget to assign focus
 * to a widget that is semantically owned by the first widget even though
 * it's not a direct child - for instance, a search entry in a floating
 * window similar to the quick search in #BtkTreeView.
 *
 * An example of its usage is:
 *
 * |[
 *   BdkEvent *fevent = bdk_event_new (BDK_FOCUS_CHANGE);
 *
 *   fevent->focus_change.type = BDK_FOCUS_CHANGE;
 *   fevent->focus_change.in = TRUE;
 *   fevent->focus_change.window = btk_widget_get_window (widget);
 *   if (fevent->focus_change.window != NULL)
 *     g_object_ref (fevent->focus_change.window);
 *
 *   btk_widget_send_focus_change (widget, fevent);
 *
 *   bdk_event_free (event);
 * ]|
 *
 * Return value: the return value from the event signal emission: %TRUE
 *   if the event was handled, and %FALSE otherwise
 *
 * Since: 2.22
 */
gboolean
btk_widget_send_focus_change (BtkWidget *widget,
                              BdkEvent  *event)
{
  gboolean res;

  g_return_val_if_fail (BTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (event != NULL && event->type == BDK_FOCUS_CHANGE, FALSE);

  g_object_ref (widget);

  _btk_widget_set_has_focus (widget, event->focus_change.in);

  res = btk_widget_event (widget, event);

  g_object_notify (G_OBJECT (widget), "has-focus");

  g_object_unref (widget);

  return res;
}

#define __BTK_WIDGET_C__
#include "btkaliasdef.c"
