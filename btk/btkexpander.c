/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */

#include "config.h"
#include <string.h>
#include "btkexpander.h"

#include "btklabel.h"
#include "btkbuildable.h"
#include "btkcontainer.h"
#include "btkmarshalers.h"
#include "btkmain.h"
#include "btkintl.h"
#include "btkprivate.h"
#include <bdk/bdkkeysyms.h>
#include "btkdnd.h"
#include "btkalias.h"

#define BTK_EXPANDER_GET_PRIVATE(o) (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_EXPANDER, BtkExpanderPrivate))

#define DEFAULT_EXPANDER_SIZE 10
#define DEFAULT_EXPANDER_SPACING 2

enum
{
  PROP_0,
  PROP_EXPANDED,
  PROP_LABEL,
  PROP_USE_UNDERLINE,
  PROP_USE_MARKUP,
  PROP_SPACING,
  PROP_LABEL_WIDGET,
  PROP_LABEL_FILL
};

struct _BtkExpanderPrivate
{
  BtkWidget        *label_widget;
  BdkWindow        *event_window;
  gint              spacing;

  BtkExpanderStyle  expander_style;
  guint             animation_timeout;
  guint             expand_timer;

  guint             expanded : 1;
  guint             use_underline : 1;
  guint             use_markup : 1; 
  guint             button_down : 1;
  guint             prelight : 1;
  guint             label_fill : 1;
};

static void btk_expander_set_property (BObject          *object,
				       guint             prop_id,
				       const BValue     *value,
				       BParamSpec       *pspec);
static void btk_expander_get_property (BObject          *object,
				       guint             prop_id,
				       BValue           *value,
				       BParamSpec       *pspec);

static void btk_expander_destroy (BtkObject *object);

static void     btk_expander_realize        (BtkWidget        *widget);
static void     btk_expander_unrealize      (BtkWidget        *widget);
static void     btk_expander_size_request   (BtkWidget        *widget,
					     BtkRequisition   *requisition);
static void     btk_expander_size_allocate  (BtkWidget        *widget,
					     BtkAllocation    *allocation);
static void     btk_expander_map            (BtkWidget        *widget);
static void     btk_expander_unmap          (BtkWidget        *widget);
static gboolean btk_expander_expose         (BtkWidget        *widget,
					     BdkEventExpose   *event);
static gboolean btk_expander_button_press   (BtkWidget        *widget,
					     BdkEventButton   *event);
static gboolean btk_expander_button_release (BtkWidget        *widget,
					     BdkEventButton   *event);
static gboolean btk_expander_enter_notify   (BtkWidget        *widget,
					     BdkEventCrossing *event);
static gboolean btk_expander_leave_notify   (BtkWidget        *widget,
					     BdkEventCrossing *event);
static gboolean btk_expander_focus          (BtkWidget        *widget,
					     BtkDirectionType  direction);
static void     btk_expander_grab_notify    (BtkWidget        *widget,
					     gboolean          was_grabbed);
static void     btk_expander_state_changed  (BtkWidget        *widget,
					     BtkStateType      previous_state);
static gboolean btk_expander_drag_motion    (BtkWidget        *widget,
					     BdkDragContext   *context,
					     gint              x,
					     gint              y,
					     guint             time);
static void     btk_expander_drag_leave     (BtkWidget        *widget,
					     BdkDragContext   *context,
					     guint             time);

static void btk_expander_add    (BtkContainer *container,
				 BtkWidget    *widget);
static void btk_expander_remove (BtkContainer *container,
				 BtkWidget    *widget);
static void btk_expander_forall (BtkContainer *container,
				 gboolean        include_internals,
				 BtkCallback     callback,
				 gpointer        callback_data);

static void btk_expander_activate (BtkExpander *expander);

static void get_expander_bounds (BtkExpander  *expander,
				 BdkRectangle *rect);

/* BtkBuildable */
static void btk_expander_buildable_init           (BtkBuildableIface *iface);
static void btk_expander_buildable_add_child      (BtkBuildable *buildable,
						   BtkBuilder   *builder,
						   BObject      *child,
						   const gchar  *type);

G_DEFINE_TYPE_WITH_CODE (BtkExpander, btk_expander, BTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_expander_buildable_init))

static void
btk_expander_class_init (BtkExpanderClass *klass)
{
  BObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  bobject_class   = (BObjectClass *) klass;
  object_class    = (BtkObjectClass *) klass;
  widget_class    = (BtkWidgetClass *) klass;
  container_class = (BtkContainerClass *) klass;

  bobject_class->set_property = btk_expander_set_property;
  bobject_class->get_property = btk_expander_get_property;

  object_class->destroy = btk_expander_destroy;

  widget_class->realize              = btk_expander_realize;
  widget_class->unrealize            = btk_expander_unrealize;
  widget_class->size_request         = btk_expander_size_request;
  widget_class->size_allocate        = btk_expander_size_allocate;
  widget_class->map                  = btk_expander_map;
  widget_class->unmap                = btk_expander_unmap;
  widget_class->expose_event         = btk_expander_expose;
  widget_class->button_press_event   = btk_expander_button_press;
  widget_class->button_release_event = btk_expander_button_release;
  widget_class->enter_notify_event   = btk_expander_enter_notify;
  widget_class->leave_notify_event   = btk_expander_leave_notify;
  widget_class->focus                = btk_expander_focus;
  widget_class->grab_notify          = btk_expander_grab_notify;
  widget_class->state_changed        = btk_expander_state_changed;
  widget_class->drag_motion          = btk_expander_drag_motion;
  widget_class->drag_leave           = btk_expander_drag_leave;

  container_class->add    = btk_expander_add;
  container_class->remove = btk_expander_remove;
  container_class->forall = btk_expander_forall;

  klass->activate = btk_expander_activate;

  g_type_class_add_private (klass, sizeof (BtkExpanderPrivate));

  g_object_class_install_property (bobject_class,
				   PROP_EXPANDED,
				   g_param_spec_boolean ("expanded",
							 P_("Expanded"),
							 P_("Whether the expander has been opened to reveal the child widget"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
				   PROP_LABEL,
				   g_param_spec_string ("label",
							P_("Label"),
							P_("Text of the expander's label"),
							NULL,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
				   PROP_USE_UNDERLINE,
				   g_param_spec_boolean ("use-underline",
							 P_("Use underline"),
							 P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
				   PROP_USE_MARKUP,
				   g_param_spec_boolean ("use-markup",
							 P_("Use markup"),
							 P_("The text of the label includes XML markup. See bango_parse_markup()"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (bobject_class,
				   PROP_SPACING,
				   g_param_spec_int ("spacing",
						     P_("Spacing"),
						     P_("Space to put between the label and the child"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_LABEL_WIDGET,
				   g_param_spec_object ("label-widget",
							P_("Label widget"),
							P_("A widget to display in place of the usual expander label"),
							BTK_TYPE_WIDGET,
							BTK_PARAM_READWRITE));

  g_object_class_install_property (bobject_class,
				   PROP_LABEL_FILL,
				   g_param_spec_boolean ("label-fill",
							 P_("Label fill"),
							 P_("Whether the label widget should fill all available horizontal space"),
							 FALSE,
							 BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("expander-size",
							     P_("Expander Size"),
							     P_("Size of the expander arrow"),
							     0,
							     G_MAXINT,
							     DEFAULT_EXPANDER_SIZE,
							     BTK_PARAM_READABLE));

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("expander-spacing",
							     P_("Indicator Spacing"),
							     P_("Spacing around expander arrow"),
							     0,
							     G_MAXINT,
							     DEFAULT_EXPANDER_SPACING,
							     BTK_PARAM_READABLE));

  widget_class->activate_signal =
    g_signal_new (I_("activate"),
		  B_TYPE_FROM_CLASS (bobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkExpanderClass, activate),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
}

static void
btk_expander_init (BtkExpander *expander)
{
  BtkExpanderPrivate *priv;

  expander->priv = priv = BTK_EXPANDER_GET_PRIVATE (expander);

  btk_widget_set_can_focus (BTK_WIDGET (expander), TRUE);
  btk_widget_set_has_window (BTK_WIDGET (expander), FALSE);

  priv->label_widget = NULL;
  priv->event_window = NULL;
  priv->spacing = 0;

  priv->expander_style = BTK_EXPANDER_COLLAPSED;
  priv->animation_timeout = 0;

  priv->expanded = FALSE;
  priv->use_underline = FALSE;
  priv->use_markup = FALSE;
  priv->button_down = FALSE;
  priv->prelight = FALSE;
  priv->label_fill = FALSE;
  priv->expand_timer = 0;

  btk_drag_dest_set (BTK_WIDGET (expander), 0, NULL, 0, 0);
  btk_drag_dest_set_track_motion (BTK_WIDGET (expander), TRUE);
}

static void
btk_expander_buildable_add_child (BtkBuildable  *buildable,
				  BtkBuilder    *builder,
				  BObject       *child,
				  const gchar   *type)
{
  if (!type)
    btk_container_add (BTK_CONTAINER (buildable), BTK_WIDGET (child));
  else if (strcmp (type, "label") == 0)
    btk_expander_set_label_widget (BTK_EXPANDER (buildable), BTK_WIDGET (child));
  else
    BTK_BUILDER_WARN_INVALID_CHILD_TYPE (BTK_EXPANDER (buildable), type);
}

static void
btk_expander_buildable_init (BtkBuildableIface *iface)
{
  iface->add_child = btk_expander_buildable_add_child;
}

static void
btk_expander_set_property (BObject      *object,
			   guint         prop_id,
			   const BValue *value,
			   BParamSpec   *pspec)
{
  BtkExpander *expander = BTK_EXPANDER (object);
                                                                                                             
  switch (prop_id)
    {
    case PROP_EXPANDED:
      btk_expander_set_expanded (expander, b_value_get_boolean (value));
      break;
    case PROP_LABEL:
      btk_expander_set_label (expander, b_value_get_string (value));
      break;
    case PROP_USE_UNDERLINE:
      btk_expander_set_use_underline (expander, b_value_get_boolean (value));
      break;
    case PROP_USE_MARKUP:
      btk_expander_set_use_markup (expander, b_value_get_boolean (value));
      break;
    case PROP_SPACING:
      btk_expander_set_spacing (expander, b_value_get_int (value));
      break;
    case PROP_LABEL_WIDGET:
      btk_expander_set_label_widget (expander, b_value_get_object (value));
      break;
    case PROP_LABEL_FILL:
      btk_expander_set_label_fill (expander, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_expander_get_property (BObject    *object,
			   guint       prop_id,
			   BValue     *value,
			   BParamSpec *pspec)
{
  BtkExpander *expander = BTK_EXPANDER (object);
  BtkExpanderPrivate *priv = expander->priv;

  switch (prop_id)
    {
    case PROP_EXPANDED:
      b_value_set_boolean (value, priv->expanded);
      break;
    case PROP_LABEL:
      b_value_set_string (value, btk_expander_get_label (expander));
      break;
    case PROP_USE_UNDERLINE:
      b_value_set_boolean (value, priv->use_underline);
      break;
    case PROP_USE_MARKUP:
      b_value_set_boolean (value, priv->use_markup);
      break;
    case PROP_SPACING:
      b_value_set_int (value, priv->spacing);
      break;
    case PROP_LABEL_WIDGET:
      b_value_set_object (value,
			  priv->label_widget ?
			  B_OBJECT (priv->label_widget) : NULL);
      break;
    case PROP_LABEL_FILL:
      b_value_set_boolean (value, priv->label_fill);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_expander_destroy (BtkObject *object)
{
  BtkExpanderPrivate *priv = BTK_EXPANDER (object)->priv;
  
  if (priv->animation_timeout)
    {
      g_source_remove (priv->animation_timeout);
      priv->animation_timeout = 0;
    }
  
  BTK_OBJECT_CLASS (btk_expander_parent_class)->destroy (object);
}

static void
btk_expander_realize (BtkWidget *widget)
{
  BtkExpanderPrivate *priv;
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;
  BdkRectangle expander_rect;
  gint label_height;

  priv = BTK_EXPANDER (widget)->priv;
  btk_widget_set_realized (widget, TRUE);

  border_width = BTK_CONTAINER (widget)->border_width;

  get_expander_bounds (BTK_EXPANDER (widget), &expander_rect);
  
  if (priv->label_widget && btk_widget_get_visible (priv->label_widget))
    {
      BtkRequisition label_requisition;

      btk_widget_get_child_requisition (priv->label_widget, &label_requisition);
      label_height = label_requisition.height;
    }
  else
    label_height = 0;

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = MAX (widget->allocation.width - 2 * border_width, 1);
  attributes.height = MAX (expander_rect.height, label_height - 2 * border_width);
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = btk_widget_get_events (widget)     |
				BDK_BUTTON_PRESS_MASK        |
				BDK_BUTTON_RELEASE_MASK      |
				BDK_ENTER_NOTIFY_MASK        |
				BDK_LEAVE_NOTIFY_MASK;

  attributes_mask = BDK_WA_X | BDK_WA_Y;

  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);

  priv->event_window = bdk_window_new (btk_widget_get_parent_window (widget),
				       &attributes, attributes_mask);
  bdk_window_set_user_data (priv->event_window, widget);

  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
btk_expander_unrealize (BtkWidget *widget)
{
  BtkExpanderPrivate *priv = BTK_EXPANDER (widget)->priv;

  if (priv->event_window)
    {
      bdk_window_set_user_data (priv->event_window, NULL);
      bdk_window_destroy (priv->event_window);
      priv->event_window = NULL;
    }

  BTK_WIDGET_CLASS (btk_expander_parent_class)->unrealize (widget);
}

static void
btk_expander_size_request (BtkWidget      *widget,
			   BtkRequisition *requisition)
{
  BtkExpander *expander;
  BtkBin *bin;
  BtkExpanderPrivate *priv;
  gint border_width;
  gint expander_size;
  gint expander_spacing;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;

  bin = BTK_BIN (widget);
  expander = BTK_EXPANDER (widget);
  priv = expander->priv;

  border_width = BTK_CONTAINER (widget)->border_width;

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  requisition->width = expander_size + 2 * expander_spacing +
		       2 * focus_width + 2 * focus_pad;
  requisition->height = interior_focus ? (2 * focus_width + 2 * focus_pad) : 0;

  if (priv->label_widget && btk_widget_get_visible (priv->label_widget))
    {
      BtkRequisition label_requisition;

      btk_widget_size_request (priv->label_widget, &label_requisition);

      requisition->width  += label_requisition.width;
      requisition->height += label_requisition.height;
    }

  requisition->height = MAX (expander_size + 2 * expander_spacing, requisition->height);

  if (!interior_focus)
    requisition->height += 2 * focus_width + 2 * focus_pad;

  if (bin->child && BTK_WIDGET_CHILD_VISIBLE (bin->child))
    {
      BtkRequisition child_requisition;

      btk_widget_size_request (bin->child, &child_requisition);

      requisition->width = MAX (requisition->width, child_requisition.width);
      requisition->height += child_requisition.height + priv->spacing;
    }

  requisition->width  += 2 * border_width;
  requisition->height += 2 * border_width;
}

static void
get_expander_bounds (BtkExpander  *expander,
		     BdkRectangle *rect)
{
  BtkWidget *widget;
  BtkExpanderPrivate *priv;
  gint border_width;
  gint expander_size;
  gint expander_spacing;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;
  gboolean ltr;

  widget = BTK_WIDGET (expander);
  priv = expander->priv;

  border_width = BTK_CONTAINER (expander)->border_width;

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  ltr = btk_widget_get_direction (widget) != BTK_TEXT_DIR_RTL;

  rect->x = widget->allocation.x + border_width;
  rect->y = widget->allocation.y + border_width;

  if (ltr)
    rect->x += expander_spacing;
  else
    rect->x += widget->allocation.width - 2 * border_width -
               expander_spacing - expander_size;

  if (priv->label_widget && btk_widget_get_visible (priv->label_widget))
    {
      BtkAllocation label_allocation;

      label_allocation = priv->label_widget->allocation;

      if (expander_size < label_allocation.height)
	rect->y += focus_width + focus_pad + (label_allocation.height - expander_size) / 2;
      else
	rect->y += expander_spacing;
    }
  else
    {
      rect->y += expander_spacing;
    }

  if (!interior_focus)
    {
      if (ltr)
	rect->x += focus_width + focus_pad;
      else
	rect->x -= focus_width + focus_pad;
      rect->y += focus_width + focus_pad;
    }

  rect->width = rect->height = expander_size;
}

static void
btk_expander_size_allocate (BtkWidget     *widget,
			    BtkAllocation *allocation)
{
  BtkExpander *expander;
  BtkBin *bin;
  BtkExpanderPrivate *priv;
  BtkRequisition child_requisition;
  gboolean child_visible = FALSE;
  gint border_width;
  gint expander_size;
  gint expander_spacing;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;
  gint label_height;

  expander = BTK_EXPANDER (widget);
  bin = BTK_BIN (widget);
  priv = expander->priv;

  border_width = BTK_CONTAINER (widget)->border_width;

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  child_requisition.width = 0;
  child_requisition.height = 0;
  if (bin->child && BTK_WIDGET_CHILD_VISIBLE (bin->child))
    {
      child_visible = TRUE;
      btk_widget_get_child_requisition (bin->child, &child_requisition);
    }

  widget->allocation = *allocation;

  if (priv->label_widget && btk_widget_get_visible (priv->label_widget))
    {
      BtkAllocation label_allocation;
      BtkRequisition label_requisition;
      gboolean ltr;

      btk_widget_get_child_requisition (priv->label_widget, &label_requisition);

      ltr = btk_widget_get_direction (widget) != BTK_TEXT_DIR_RTL;

      if (priv->label_fill)
	label_allocation.x = (widget->allocation.x +
                              border_width + focus_width + focus_pad +
                              expander_size + 2 * expander_spacing);
      else if (ltr)
	label_allocation.x = (widget->allocation.x +
                              border_width + focus_width + focus_pad +
                              expander_size + 2 * expander_spacing);
      else
        label_allocation.x = (widget->allocation.x + widget->allocation.width -
                              (label_requisition.width +
                               border_width + focus_width + focus_pad +
                               expander_size + 2 * expander_spacing));

      label_allocation.y = widget->allocation.y + border_width + focus_width + focus_pad;

      if (priv->label_fill)
        label_allocation.width = allocation->width - 2 * border_width -
				 expander_size - 2 * expander_spacing -
				 2 * focus_width - 2 * focus_pad;
      else
        label_allocation.width = MIN (label_requisition.width,
				      allocation->width - 2 * border_width -
				      expander_size - 2 * expander_spacing -
				      2 * focus_width - 2 * focus_pad);
      label_allocation.width = MAX (label_allocation.width, 1);

      label_allocation.height = MIN (label_requisition.height,
				     allocation->height - 2 * border_width -
				     2 * focus_width - 2 * focus_pad -
				     (child_visible ? priv->spacing : 0));
      label_allocation.height = MAX (label_allocation.height, 1);

      btk_widget_size_allocate (priv->label_widget, &label_allocation);

      label_height = label_allocation.height;
    }
  else
    {
      label_height = 0;
    }

  if (btk_widget_get_realized (widget))
    {
      BdkRectangle rect;

      get_expander_bounds (expander, &rect);

      bdk_window_move_resize (priv->event_window,
			      allocation->x + border_width,
			      allocation->y + border_width,
			      MAX (allocation->width - 2 * border_width, 1),
			      MAX (rect.height, label_height - 2 * border_width));
    }

  if (child_visible)
    {
      BtkAllocation child_allocation;
      gint top_height;

      top_height = MAX (2 * expander_spacing + expander_size,
			label_height +
			(interior_focus ? 2 * focus_width + 2 * focus_pad : 0));

      child_allocation.x = widget->allocation.x + border_width;
      child_allocation.y = widget->allocation.y + border_width + top_height + priv->spacing;

      if (!interior_focus)
	child_allocation.y += 2 * focus_width + 2 * focus_pad;

      child_allocation.width = MAX (allocation->width - 2 * border_width, 1);

      child_allocation.height = allocation->height - top_height -
				2 * border_width - priv->spacing -
				(!interior_focus ? 2 * focus_width + 2 * focus_pad : 0);
      child_allocation.height = MAX (child_allocation.height, 1);

      btk_widget_size_allocate (bin->child, &child_allocation);
    }
}

static void
btk_expander_map (BtkWidget *widget)
{
  BtkExpanderPrivate *priv = BTK_EXPANDER (widget)->priv;

  if (priv->label_widget)
    btk_widget_map (priv->label_widget);

  BTK_WIDGET_CLASS (btk_expander_parent_class)->map (widget);

  if (priv->event_window)
    bdk_window_show (priv->event_window);
}

static void
btk_expander_unmap (BtkWidget *widget)
{
  BtkExpanderPrivate *priv = BTK_EXPANDER (widget)->priv;

  if (priv->event_window)
    bdk_window_hide (priv->event_window);

  BTK_WIDGET_CLASS (btk_expander_parent_class)->unmap (widget);

  if (priv->label_widget)
    btk_widget_unmap (priv->label_widget);
}

static void
btk_expander_paint_prelight (BtkExpander *expander)
{
  BtkWidget *widget;
  BtkContainer *container;
  BtkExpanderPrivate *priv;
  BdkRectangle area;
  gboolean interior_focus;
  int focus_width;
  int focus_pad;
  int expander_size;
  int expander_spacing;

  priv = expander->priv;
  widget = BTK_WIDGET (expander);
  container = BTK_CONTAINER (expander);

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  area.x = widget->allocation.x + container->border_width;
  area.y = widget->allocation.y + container->border_width;
  area.width = widget->allocation.width - (2 * container->border_width);

  if (priv->label_widget && btk_widget_get_visible (priv->label_widget))
    area.height = priv->label_widget->allocation.height;
  else
    area.height = 0;

  area.height += interior_focus ? (focus_width + focus_pad) * 2 : 0;
  area.height = MAX (area.height, expander_size + 2 * expander_spacing);
  area.height += !interior_focus ? (focus_width + focus_pad) * 2 : 0;

  btk_paint_flat_box (widget->style, widget->window,
		      BTK_STATE_PRELIGHT,
		      BTK_SHADOW_ETCHED_OUT,
		      &area, widget, "expander",
		      area.x, area.y,
		      area.width, area.height);
}

static void
btk_expander_paint (BtkExpander *expander)
{
  BtkWidget *widget;
  BdkRectangle clip;
  BtkStateType state;

  widget = BTK_WIDGET (expander);

  get_expander_bounds (expander, &clip);

  state = widget->state;
  if (expander->priv->prelight)
    {
      state = BTK_STATE_PRELIGHT;

      btk_expander_paint_prelight (expander);
    }

  btk_paint_expander (widget->style,
		      widget->window,
		      state,
		      &clip,
		      widget,
		      "expander",
		      clip.x + clip.width / 2,
		      clip.y + clip.height / 2,
		      expander->priv->expander_style);
}

static void
btk_expander_paint_focus (BtkExpander  *expander,
			  BdkRectangle *area)
{
  BtkWidget *widget;
  BtkExpanderPrivate *priv;
  BdkRectangle rect;
  gint x, y, width, height;
  gboolean interior_focus;
  gint border_width;
  gint focus_width;
  gint focus_pad;
  gint expander_size;
  gint expander_spacing;
  gboolean ltr;

  widget = BTK_WIDGET (expander);
  priv = expander->priv;

  border_width = BTK_CONTAINER (widget)->border_width;

  btk_widget_style_get (widget,
			"interior-focus", &interior_focus,
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			"expander-size", &expander_size,
			"expander-spacing", &expander_spacing,
			NULL);

  ltr = btk_widget_get_direction (widget) != BTK_TEXT_DIR_RTL;
  
  width = height = 0;

  if (priv->label_widget)
    {
      if (btk_widget_get_visible (priv->label_widget))
	{
	  BtkAllocation label_allocation = priv->label_widget->allocation;

	  width  = label_allocation.width;
	  height = label_allocation.height;
	}

      width  += 2 * focus_pad + 2 * focus_width;
      height += 2 * focus_pad + 2 * focus_width;

      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;

      if (ltr)
	{
	  if (interior_focus)
	    x += expander_spacing * 2 + expander_size;
	}
      else
	{
	  x += widget->allocation.width - 2 * border_width
	    - expander_spacing * 2 - expander_size - width;
	}

      if (!interior_focus)
	{
	  width += expander_size + 2 * expander_spacing;
	  height = MAX (height, expander_size + 2 * expander_spacing);
	}
    }
  else
    {
      get_expander_bounds (expander, &rect);

      x = rect.x - focus_pad;
      y = rect.y - focus_pad;
      width = rect.width + 2 * focus_pad;
      height = rect.height + 2 * focus_pad;
    }
      
  btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
		   area, widget, "expander",
		   x, y, width, height);
}

static gboolean
btk_expander_expose (BtkWidget      *widget,
		     BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      BtkExpander *expander = BTK_EXPANDER (widget);

      btk_expander_paint (expander);

      if (btk_widget_has_focus (widget))
	btk_expander_paint_focus (expander, &event->area);

      BTK_WIDGET_CLASS (btk_expander_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static gboolean
btk_expander_button_press (BtkWidget      *widget,
			   BdkEventButton *event)
{
  BtkExpander *expander = BTK_EXPANDER (widget);

  if (event->button == 1 && event->window == expander->priv->event_window)
    {
      expander->priv->button_down = TRUE;
      return TRUE;
    }

  return FALSE;
}

static gboolean
btk_expander_button_release (BtkWidget      *widget,
			     BdkEventButton *event)
{
  BtkExpander *expander = BTK_EXPANDER (widget);

  if (event->button == 1 && expander->priv->button_down)
    {
      btk_widget_activate (widget);
      expander->priv->button_down = FALSE;
      return TRUE;
    }

  return FALSE;
}

static void
btk_expander_grab_notify (BtkWidget *widget,
			  gboolean   was_grabbed)
{
  if (!was_grabbed)
    BTK_EXPANDER (widget)->priv->button_down = FALSE;
}

static void
btk_expander_state_changed (BtkWidget    *widget,
			    BtkStateType  previous_state)
{
  if (!btk_widget_is_sensitive (widget))
    BTK_EXPANDER (widget)->priv->button_down = FALSE;
}

static void
btk_expander_redraw_expander (BtkExpander *expander)
{
  BtkWidget *widget;

  widget = BTK_WIDGET (expander);

  if (btk_widget_get_realized (widget))
    bdk_window_invalidate_rect (widget->window, &widget->allocation, FALSE);
}

static gboolean
btk_expander_enter_notify (BtkWidget        *widget,
			   BdkEventCrossing *event)
{
  BtkExpander *expander = BTK_EXPANDER (widget);
  BtkWidget *event_widget;

  event_widget = btk_get_event_widget ((BdkEvent *) event);

  if (event_widget == widget &&
      event->detail != BDK_NOTIFY_INFERIOR)
    {
      expander->priv->prelight = TRUE;

      if (expander->priv->label_widget)
	btk_widget_set_state (expander->priv->label_widget, BTK_STATE_PRELIGHT);

      btk_expander_redraw_expander (expander);
    }

  return FALSE;
}

static gboolean
btk_expander_leave_notify (BtkWidget        *widget,
			   BdkEventCrossing *event)
{
  BtkExpander *expander = BTK_EXPANDER (widget);
  BtkWidget *event_widget;

  event_widget = btk_get_event_widget ((BdkEvent *) event);

  if (event_widget == widget &&
      event->detail != BDK_NOTIFY_INFERIOR)
    {
      expander->priv->prelight = FALSE;

      if (expander->priv->label_widget)
	btk_widget_set_state (expander->priv->label_widget, BTK_STATE_NORMAL);

      btk_expander_redraw_expander (expander);
    }

  return FALSE;
}

static gboolean
expand_timeout (gpointer data)
{
  BtkExpander *expander = BTK_EXPANDER (data);
  BtkExpanderPrivate *priv = expander->priv;

  priv->expand_timer = 0;
  btk_expander_set_expanded (expander, TRUE);

  return FALSE;
}

static gboolean
btk_expander_drag_motion (BtkWidget        *widget,
			  BdkDragContext   *context,
			  gint              x,
			  gint              y,
			  guint             time)
{
  BtkExpander *expander = BTK_EXPANDER (widget);
  BtkExpanderPrivate *priv = expander->priv;

  if (!priv->expanded && !priv->expand_timer)
    {
      BtkSettings *settings;
      guint timeout;

      settings = btk_widget_get_settings (widget);
      g_object_get (settings, "btk-timeout-expand", &timeout, NULL);

      priv->expand_timer = bdk_threads_add_timeout (timeout, (GSourceFunc) expand_timeout, expander);
    }

  return TRUE;
}

static void
btk_expander_drag_leave (BtkWidget      *widget,
			 BdkDragContext *context,
			 guint           time)
{
  BtkExpander *expander = BTK_EXPANDER (widget);
  BtkExpanderPrivate *priv = expander->priv;

  if (priv->expand_timer)
    {
      g_source_remove (priv->expand_timer);
      priv->expand_timer = 0;
    }
}

typedef enum
{
  FOCUS_NONE,
  FOCUS_WIDGET,
  FOCUS_LABEL,
  FOCUS_CHILD
} FocusSite;

static gboolean
focus_current_site (BtkExpander      *expander,
		    BtkDirectionType  direction)
{
  BtkWidget *current_focus;

  current_focus = BTK_CONTAINER (expander)->focus_child;

  if (!current_focus)
    return FALSE;

  return btk_widget_child_focus (current_focus, direction);
}

static gboolean
focus_in_site (BtkExpander      *expander,
	       FocusSite         site,
	       BtkDirectionType  direction)
{
  switch (site)
    {
    case FOCUS_WIDGET:
      btk_widget_grab_focus (BTK_WIDGET (expander));
      return TRUE;
    case FOCUS_LABEL:
      if (expander->priv->label_widget)
	return btk_widget_child_focus (expander->priv->label_widget, direction);
      else
	return FALSE;
    case FOCUS_CHILD:
      {
	BtkWidget *child = btk_bin_get_child (BTK_BIN (expander));

	if (child && BTK_WIDGET_CHILD_VISIBLE (child))
	  return btk_widget_child_focus (child, direction);
	else
	  return FALSE;
      }
    case FOCUS_NONE:
      break;
    }

  g_assert_not_reached ();
  return FALSE;
}

static FocusSite
get_next_site (BtkExpander      *expander,
	       FocusSite         site,
	       BtkDirectionType  direction)
{
  gboolean ltr;

  ltr = btk_widget_get_direction (BTK_WIDGET (expander)) != BTK_TEXT_DIR_RTL;

  switch (site)
    {
    case FOCUS_NONE:
      switch (direction)
	{
	case BTK_DIR_TAB_BACKWARD:
	case BTK_DIR_LEFT:
	case BTK_DIR_UP:
	  return FOCUS_CHILD;
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_DOWN:
	case BTK_DIR_RIGHT:
	  return FOCUS_WIDGET;
	}
    case FOCUS_WIDGET:
      switch (direction)
	{
	case BTK_DIR_TAB_BACKWARD:
	case BTK_DIR_UP:
	  return FOCUS_NONE;
	case BTK_DIR_LEFT:
	  return ltr ? FOCUS_NONE : FOCUS_LABEL;
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_DOWN:
	  return FOCUS_LABEL;
	case BTK_DIR_RIGHT:
	  return ltr ? FOCUS_LABEL : FOCUS_NONE;
	  break;
	}
    case FOCUS_LABEL:
      switch (direction)
	{
	case BTK_DIR_TAB_BACKWARD:
	case BTK_DIR_UP:
	  return FOCUS_WIDGET;
	case BTK_DIR_LEFT:
	  return ltr ? FOCUS_WIDGET : FOCUS_CHILD;
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_DOWN:
	  return FOCUS_CHILD;
	case BTK_DIR_RIGHT:
	  return ltr ? FOCUS_CHILD : FOCUS_WIDGET;
	  break;
	}
    case FOCUS_CHILD:
      switch (direction)
	{
	case BTK_DIR_TAB_BACKWARD:
	case BTK_DIR_LEFT:
	case BTK_DIR_UP:
	  return FOCUS_LABEL;
	case BTK_DIR_TAB_FORWARD:
	case BTK_DIR_DOWN:
	case BTK_DIR_RIGHT:
	  return FOCUS_NONE;
	}
    }

  g_assert_not_reached ();
  return FOCUS_NONE;
}

static gboolean
btk_expander_focus (BtkWidget        *widget,
		    BtkDirectionType  direction)
{
  BtkExpander *expander = BTK_EXPANDER (widget);
  
  if (!focus_current_site (expander, direction))
    {
      BtkWidget *old_focus_child;
      gboolean widget_is_focus;
      FocusSite site = FOCUS_NONE;
      
      widget_is_focus = btk_widget_is_focus (widget);
      old_focus_child = BTK_CONTAINER (widget)->focus_child;
      
      if (old_focus_child && old_focus_child == expander->priv->label_widget)
	site = FOCUS_LABEL;
      else if (old_focus_child)
	site = FOCUS_CHILD;
      else if (widget_is_focus)
	site = FOCUS_WIDGET;

      while ((site = get_next_site (expander, site, direction)) != FOCUS_NONE)
	{
	  if (focus_in_site (expander, site, direction))
	    return TRUE;
	}

      return FALSE;
    }

  return TRUE;
}

static void
btk_expander_add (BtkContainer *container,
		  BtkWidget    *widget)
{
  BTK_CONTAINER_CLASS (btk_expander_parent_class)->add (container, widget);

  btk_widget_set_child_visible (widget, BTK_EXPANDER (container)->priv->expanded);
  btk_widget_queue_resize (BTK_WIDGET (container));
}

static void
btk_expander_remove (BtkContainer *container,
		     BtkWidget    *widget)
{
  BtkExpander *expander = BTK_EXPANDER (container);

  if (BTK_EXPANDER (expander)->priv->label_widget == widget)
    btk_expander_set_label_widget (expander, NULL);
  else
    BTK_CONTAINER_CLASS (btk_expander_parent_class)->remove (container, widget);
}

static void
btk_expander_forall (BtkContainer *container,
		     gboolean      include_internals,
		     BtkCallback   callback,
		     gpointer      callback_data)
{
  BtkBin *bin = BTK_BIN (container);
  BtkExpanderPrivate *priv = BTK_EXPANDER (container)->priv;

  if (bin->child)
    (* callback) (bin->child, callback_data);

  if (priv->label_widget)
    (* callback) (priv->label_widget, callback_data);
}

static void
btk_expander_activate (BtkExpander *expander)
{
  btk_expander_set_expanded (expander, !expander->priv->expanded);
}

/**
 * btk_expander_new:
 * @label: the text of the label
 * 
 * Creates a new expander using @label as the text of the label.
 * 
 * Return value: a new #BtkExpander widget.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_expander_new (const gchar *label)
{
  return g_object_new (BTK_TYPE_EXPANDER, "label", label, NULL);
}

/**
 * btk_expander_new_with_mnemonic:
 * @label: (allow-none): the text of the label with an underscore in front of the
 *         mnemonic character
 *
 * Creates a new expander using @label as the text of the label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use '__' (two 
 * underscores). The first underlined character represents a keyboard 
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 * 
 * Return value: a new #BtkExpander widget.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_expander_new_with_mnemonic (const gchar *label)
{
  return g_object_new (BTK_TYPE_EXPANDER,
		       "label", label,
		       "use-underline", TRUE,
		       NULL);
}

static gboolean
btk_expander_animation_timeout (BtkExpander *expander)
{
  BtkExpanderPrivate *priv = expander->priv;
  BdkRectangle area;
  gboolean finish = FALSE;

  if (btk_widget_get_realized (BTK_WIDGET (expander)))
    {
      get_expander_bounds (expander, &area);
      bdk_window_invalidate_rect (BTK_WIDGET (expander)->window, &area, TRUE);
    }

  if (priv->expanded)
    {
      if (priv->expander_style == BTK_EXPANDER_COLLAPSED)
	{
	  priv->expander_style = BTK_EXPANDER_SEMI_EXPANDED;
	}
      else
	{
	  priv->expander_style = BTK_EXPANDER_EXPANDED;
	  finish = TRUE;
	}
    }
  else
    {
      if (priv->expander_style == BTK_EXPANDER_EXPANDED)
	{
	  priv->expander_style = BTK_EXPANDER_SEMI_COLLAPSED;
	}
      else
	{
	  priv->expander_style = BTK_EXPANDER_COLLAPSED;
	  finish = TRUE;
	}
    }

  if (finish)
    {
      priv->animation_timeout = 0;
      if (BTK_BIN (expander)->child)
	btk_widget_set_child_visible (BTK_BIN (expander)->child, priv->expanded);
      btk_widget_queue_resize (BTK_WIDGET (expander));
    }

  return !finish;
}

static void
btk_expander_start_animation (BtkExpander *expander)
{
  BtkExpanderPrivate *priv = expander->priv;

  if (priv->animation_timeout)
    g_source_remove (priv->animation_timeout);

  priv->animation_timeout =
		bdk_threads_add_timeout (50,
			       (GSourceFunc) btk_expander_animation_timeout,
			       expander);
}

/**
 * btk_expander_set_expanded:
 * @expander: a #BtkExpander
 * @expanded: whether the child widget is revealed
 *
 * Sets the state of the expander. Set to %TRUE, if you want
 * the child widget to be revealed, and %FALSE if you want the
 * child widget to be hidden.
 *
 * Since: 2.4
 **/
void
btk_expander_set_expanded (BtkExpander *expander,
			   gboolean     expanded)
{
  BtkExpanderPrivate *priv;

  g_return_if_fail (BTK_IS_EXPANDER (expander));

  priv = expander->priv;

  expanded = expanded != FALSE;

  if (priv->expanded != expanded)
    {
      BtkSettings *settings = btk_widget_get_settings (BTK_WIDGET (expander));
      gboolean     enable_animations;

      priv->expanded = expanded;

      g_object_get (settings, "btk-enable-animations", &enable_animations, NULL);

      if (enable_animations && btk_widget_get_realized (BTK_WIDGET (expander)))
	{
	  btk_expander_start_animation (expander);
	}
      else
	{
	  priv->expander_style = expanded ? BTK_EXPANDER_EXPANDED :
					    BTK_EXPANDER_COLLAPSED;

	  if (BTK_BIN (expander)->child)
	    {
	      btk_widget_set_child_visible (BTK_BIN (expander)->child, priv->expanded);
	      btk_widget_queue_resize (BTK_WIDGET (expander));
	    }
	}

      g_object_notify (B_OBJECT (expander), "expanded");
    }
}

/**
 * btk_expander_get_expanded:
 * @expander:a #BtkExpander
 *
 * Queries a #BtkExpander and returns its current state. Returns %TRUE
 * if the child widget is revealed.
 *
 * See btk_expander_set_expanded().
 *
 * Return value: the current state of the expander.
 *
 * Since: 2.4
 **/
gboolean
btk_expander_get_expanded (BtkExpander *expander)
{
  g_return_val_if_fail (BTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->expanded;
}

/**
 * btk_expander_set_spacing:
 * @expander: a #BtkExpander
 * @spacing: distance between the expander and child in pixels.
 *
 * Sets the spacing field of @expander, which is the number of pixels to
 * place between expander and the child.
 *
 * Since: 2.4
 **/
void
btk_expander_set_spacing (BtkExpander *expander,
			  gint         spacing)
{
  g_return_if_fail (BTK_IS_EXPANDER (expander));
  g_return_if_fail (spacing >= 0);

  if (expander->priv->spacing != spacing)
    {
      expander->priv->spacing = spacing;

      btk_widget_queue_resize (BTK_WIDGET (expander));

      g_object_notify (B_OBJECT (expander), "spacing");
    }
}

/**
 * btk_expander_get_spacing:
 * @expander: a #BtkExpander
 *
 * Gets the value set by btk_expander_set_spacing().
 *
 * Return value: spacing between the expander and child.
 *
 * Since: 2.4
 **/
gint
btk_expander_get_spacing (BtkExpander *expander)
{
  g_return_val_if_fail (BTK_IS_EXPANDER (expander), 0);

  return expander->priv->spacing;
}

/**
 * btk_expander_set_label:
 * @expander: a #BtkExpander
 * @label: (allow-none): a string
 *
 * Sets the text of the label of the expander to @label.
 *
 * This will also clear any previously set labels.
 *
 * Since: 2.4
 **/
void
btk_expander_set_label (BtkExpander *expander,
			const gchar *label)
{
  g_return_if_fail (BTK_IS_EXPANDER (expander));

  if (!label)
    {
      btk_expander_set_label_widget (expander, NULL);
    }
  else
    {
      BtkWidget *child;

      child = btk_label_new (label);
      btk_label_set_use_underline (BTK_LABEL (child), expander->priv->use_underline);
      btk_label_set_use_markup (BTK_LABEL (child), expander->priv->use_markup);
      btk_widget_show (child);

      btk_expander_set_label_widget (expander, child);
    }

  g_object_notify (B_OBJECT (expander), "label");
}

/**
 * btk_expander_get_label:
 * @expander: a #BtkExpander
 *
 * Fetches the text from a label widget including any embedded
 * underlines indicating mnemonics and Bango markup, as set by
 * btk_expander_set_label(). If the label text has not been set the
 * return value will be %NULL. This will be the case if you create an
 * empty button with btk_button_new() to use as a container.
 *
 * Note that this function behaved differently in versions prior to
 * 2.14 and used to return the label text stripped of embedded
 * underlines indicating mnemonics and Bango markup. This problem can
 * be avoided by fetching the label text directly from the label
 * widget.
 *
 * Return value: The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 *
 * Since: 2.4
 **/
const char *
btk_expander_get_label (BtkExpander *expander)
{
  BtkExpanderPrivate *priv;

  g_return_val_if_fail (BTK_IS_EXPANDER (expander), NULL);

  priv = expander->priv;

  if (BTK_IS_LABEL (priv->label_widget))
    return btk_label_get_label (BTK_LABEL (priv->label_widget));
  else
    return NULL;
}

/**
 * btk_expander_set_use_underline:
 * @expander: a #BtkExpander
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the expander label indicates
 * the next character should be used for the mnemonic accelerator key.
 *
 * Since: 2.4
 **/
void
btk_expander_set_use_underline (BtkExpander *expander,
				gboolean     use_underline)
{
  BtkExpanderPrivate *priv;

  g_return_if_fail (BTK_IS_EXPANDER (expander));

  priv = expander->priv;

  use_underline = use_underline != FALSE;

  if (priv->use_underline != use_underline)
    {
      priv->use_underline = use_underline;

      if (BTK_IS_LABEL (priv->label_widget))
	btk_label_set_use_underline (BTK_LABEL (priv->label_widget), use_underline);

      g_object_notify (B_OBJECT (expander), "use-underline");
    }
}

/**
 * btk_expander_get_use_underline:
 * @expander: a #BtkExpander
 *
 * Returns whether an embedded underline in the expander label indicates a
 * mnemonic. See btk_expander_set_use_underline().
 *
 * Return value: %TRUE if an embedded underline in the expander label
 *               indicates the mnemonic accelerator keys.
 *
 * Since: 2.4
 **/
gboolean
btk_expander_get_use_underline (BtkExpander *expander)
{
  g_return_val_if_fail (BTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->use_underline;
}

/**
 * btk_expander_set_use_markup:
 * @expander: a #BtkExpander
 * @use_markup: %TRUE if the label's text should be parsed for markup
 *
 * Sets whether the text of the label contains markup in <link
 * linkend="BangoMarkupFormat">Bango's text markup
 * language</link>. See btk_label_set_markup().
 *
 * Since: 2.4
 **/
void
btk_expander_set_use_markup (BtkExpander *expander,
			     gboolean     use_markup)
{
  BtkExpanderPrivate *priv;

  g_return_if_fail (BTK_IS_EXPANDER (expander));

  priv = expander->priv;

  use_markup = use_markup != FALSE;

  if (priv->use_markup != use_markup)
    {
      priv->use_markup = use_markup;

      if (BTK_IS_LABEL (priv->label_widget))
	btk_label_set_use_markup (BTK_LABEL (priv->label_widget), use_markup);

      g_object_notify (B_OBJECT (expander), "use-markup");
    }
}

/**
 * btk_expander_get_use_markup:
 * @expander: a #BtkExpander
 *
 * Returns whether the label's text is interpreted as marked up with
 * the <link linkend="BangoMarkupFormat">Bango text markup
 * language</link>. See btk_expander_set_use_markup ().
 *
 * Return value: %TRUE if the label's text will be parsed for markup
 *
 * Since: 2.4
 **/
gboolean
btk_expander_get_use_markup (BtkExpander *expander)
{
  g_return_val_if_fail (BTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->use_markup;
}

/**
 * btk_expander_set_label_widget:
 * @expander: a #BtkExpander
 * @label_widget: (allow-none): the new label widget
 *
 * Set the label widget for the expander. This is the widget
 * that will appear embedded alongside the expander arrow.
 *
 * Since: 2.4
 **/
void
btk_expander_set_label_widget (BtkExpander *expander,
			       BtkWidget   *label_widget)
{
  BtkExpanderPrivate *priv;
  BtkWidget          *widget;

  g_return_if_fail (BTK_IS_EXPANDER (expander));
  g_return_if_fail (label_widget == NULL || BTK_IS_WIDGET (label_widget));
  g_return_if_fail (label_widget == NULL || label_widget->parent == NULL);

  priv = expander->priv;

  if (priv->label_widget == label_widget)
    return;

  if (priv->label_widget)
    {
      btk_widget_set_state (priv->label_widget, BTK_STATE_NORMAL);
      btk_widget_unparent (priv->label_widget);
    }

  priv->label_widget = label_widget;
  widget = BTK_WIDGET (expander);

  if (label_widget)
    {
      priv->label_widget = label_widget;

      btk_widget_set_parent (label_widget, widget);

      if (priv->prelight)
	btk_widget_set_state (label_widget, BTK_STATE_PRELIGHT);
    }

  if (btk_widget_get_visible (widget))
    btk_widget_queue_resize (widget);

  g_object_freeze_notify (B_OBJECT (expander));
  g_object_notify (B_OBJECT (expander), "label-widget");
  g_object_notify (B_OBJECT (expander), "label");
  g_object_thaw_notify (B_OBJECT (expander));
}

/**
 * btk_expander_get_label_widget:
 * @expander: a #BtkExpander
 *
 * Retrieves the label widget for the frame. See
 * btk_expander_set_label_widget().
 *
 * Return value: (transfer none): the label widget,
 *     or %NULL if there is none.
 *
 * Since: 2.4
 **/
BtkWidget *
btk_expander_get_label_widget (BtkExpander *expander)
{
  g_return_val_if_fail (BTK_IS_EXPANDER (expander), NULL);

  return expander->priv->label_widget;
}

/**
 * btk_expander_set_label_fill:
 * @expander: a #BtkExpander
 * @label_fill: %TRUE if the label should should fill all available horizontal
 *              space
 *
 * Sets whether the label widget should fill all available horizontal space
 * allocated to @expander.
 *
 * Since: 2.22
 */
void
btk_expander_set_label_fill (BtkExpander *expander,
                             gboolean     label_fill)
{
  BtkExpanderPrivate *priv;

  g_return_if_fail (BTK_IS_EXPANDER (expander));

  priv = expander->priv;

  label_fill = label_fill != FALSE;

  if (priv->label_fill != label_fill)
    {
      priv->label_fill = label_fill;

      if (priv->label_widget != NULL)
        btk_widget_queue_resize (BTK_WIDGET (expander));

      g_object_notify (B_OBJECT (expander), "label-fill");
    }
}

/**
 * btk_expander_get_label_fill:
 * @expander: a #BtkExpander
 *
 * Returns whether the label widget will fill all available horizontal
 * space allocated to @expander.
 *
 * Return value: %TRUE if the label widget will fill all available horizontal
 *               space
 *
 * Since: 2.22
 */
gboolean
btk_expander_get_label_fill (BtkExpander *expander)
{
  g_return_val_if_fail (BTK_IS_EXPANDER (expander), FALSE);

  return expander->priv->label_fill;
}

#define __BTK_EXPANDER_C__
#include "btkaliasdef.c"

