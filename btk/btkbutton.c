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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <string.h>
#include "btkalignment.h"
#include "btkbutton.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkmarshalers.h"
#include "btkimage.h"
#include "btkhbox.h"
#include "btkvbox.h"
#include "btkstock.h"
#include "btkiconfactory.h"
#include "btkactivatable.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

static const BtkBorder default_default_border = { 1, 1, 1, 1 };
static const BtkBorder default_default_outside_border = { 0, 0, 0, 0 };
static const BtkBorder default_inner_border = { 1, 1, 1, 1 };

/* Time out before giving up on getting a key release when animating
 * the close button.
 */
#define ACTIVATE_TIMEOUT 250

enum {
  PRESSED,
  RELEASED,
  CLICKED,
  ENTER,
  LEAVE,
  ACTIVATE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_IMAGE,
  PROP_RELIEF,
  PROP_USE_UNDERLINE,
  PROP_USE_STOCK,
  PROP_FOCUS_ON_CLICK,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_IMAGE_POSITION,

  /* activatable properties */
  PROP_ACTIVATABLE_RELATED_ACTION,
  PROP_ACTIVATABLE_USE_ACTION_APPEARANCE
};

#define BTK_BUTTON_GET_PRIVATE(o)       (B_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_BUTTON, BtkButtonPrivate))
typedef struct _BtkButtonPrivate BtkButtonPrivate;

struct _BtkButtonPrivate
{
  gfloat          xalign;
  gfloat          yalign;
  BtkWidget      *image;
  guint           align_set             : 1;
  guint           image_is_stock        : 1;
  guint           has_grab              : 1;
  guint           use_action_appearance : 1;
  guint32         grab_time;
  BtkPositionType image_position;
  BtkAction      *action;
};

static void btk_button_destroy        (BtkObject          *object);
static void btk_button_dispose        (BObject            *object);
static void btk_button_set_property   (BObject            *object,
                                       guint               prop_id,
                                       const BValue       *value,
                                       BParamSpec         *pspec);
static void btk_button_get_property   (BObject            *object,
                                       guint               prop_id,
                                       BValue             *value,
                                       BParamSpec         *pspec);
static void btk_button_screen_changed (BtkWidget          *widget,
				       BdkScreen          *previous_screen);
static void btk_button_realize (BtkWidget * widget);
static void btk_button_unrealize (BtkWidget * widget);
static void btk_button_map (BtkWidget * widget);
static void btk_button_unmap (BtkWidget * widget);
static void btk_button_style_set (BtkWidget * widget, BtkStyle * prev_style);
static void btk_button_size_request (BtkWidget * widget,
				     BtkRequisition * requisition);
static void btk_button_size_allocate (BtkWidget * widget,
				      BtkAllocation * allocation);
static gint btk_button_expose (BtkWidget * widget, BdkEventExpose * event);
static gint btk_button_button_press (BtkWidget * widget,
				     BdkEventButton * event);
static gint btk_button_button_release (BtkWidget * widget,
				       BdkEventButton * event);
static gint btk_button_grab_broken (BtkWidget * widget,
				    BdkEventGrabBroken * event);
static gint btk_button_key_release (BtkWidget * widget, BdkEventKey * event);
static gint btk_button_enter_notify (BtkWidget * widget,
				     BdkEventCrossing * event);
static gint btk_button_leave_notify (BtkWidget * widget,
				     BdkEventCrossing * event);
static void btk_real_button_pressed (BtkButton * button);
static void btk_real_button_released (BtkButton * button);
static void btk_real_button_clicked (BtkButton * button);
static void btk_real_button_activate  (BtkButton          *button);
static void btk_button_update_state   (BtkButton          *button);
static void btk_button_add            (BtkContainer       *container,
			               BtkWidget          *widget);
static GType btk_button_child_type    (BtkContainer       *container);
static void btk_button_finish_activate (BtkButton         *button,
					gboolean           do_it);

static BObject*	btk_button_constructor (GType                  type,
					guint                  n_construct_properties,
					BObjectConstructParam *construct_params);
static void btk_button_construct_child (BtkButton             *button);
static void btk_button_state_changed   (BtkWidget             *widget,
					BtkStateType           previous_state);
static void btk_button_grab_notify     (BtkWidget             *widget,
					gboolean               was_grabbed);


static void btk_button_activatable_interface_init         (BtkActivatableIface  *iface);
static void btk_button_update                    (BtkActivatable       *activatable,
				                  BtkAction            *action,
			                          const gchar          *property_name);
static void btk_button_sync_action_properties    (BtkActivatable       *activatable,
                                                  BtkAction            *action);
static void btk_button_set_related_action        (BtkButton            *button,
					          BtkAction            *action);
static void btk_button_set_use_action_appearance (BtkButton            *button,
						  gboolean              use_appearance);

static guint button_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkButton, btk_button, BTK_TYPE_BIN,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_ACTIVATABLE,
						btk_button_activatable_interface_init))

static void
btk_button_class_init (BtkButtonClass *klass)
{
  BObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  BtkContainerClass *container_class;

  bobject_class = B_OBJECT_CLASS (klass);
  object_class = (BtkObjectClass*) klass;
  widget_class = (BtkWidgetClass*) klass;
  container_class = (BtkContainerClass*) klass;
  
  bobject_class->constructor  = btk_button_constructor;
  bobject_class->dispose      = btk_button_dispose;
  bobject_class->set_property = btk_button_set_property;
  bobject_class->get_property = btk_button_get_property;

  object_class->destroy = btk_button_destroy;

  widget_class->screen_changed = btk_button_screen_changed;
  widget_class->realize = btk_button_realize;
  widget_class->unrealize = btk_button_unrealize;
  widget_class->map = btk_button_map;
  widget_class->unmap = btk_button_unmap;
  widget_class->style_set = btk_button_style_set;
  widget_class->size_request = btk_button_size_request;
  widget_class->size_allocate = btk_button_size_allocate;
  widget_class->expose_event = btk_button_expose;
  widget_class->button_press_event = btk_button_button_press;
  widget_class->button_release_event = btk_button_button_release;
  widget_class->grab_broken_event = btk_button_grab_broken;
  widget_class->key_release_event = btk_button_key_release;
  widget_class->enter_notify_event = btk_button_enter_notify;
  widget_class->leave_notify_event = btk_button_leave_notify;
  widget_class->state_changed = btk_button_state_changed;
  widget_class->grab_notify = btk_button_grab_notify;

  container_class->child_type = btk_button_child_type;
  container_class->add = btk_button_add;

  klass->pressed = btk_real_button_pressed;
  klass->released = btk_real_button_released;
  klass->clicked = NULL;
  klass->enter = btk_button_update_state;
  klass->leave = btk_button_update_state;
  klass->activate = btk_real_button_activate;

  g_object_class_install_property (bobject_class,
                                   PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        P_("Label"),
                                                        P_("Text of the label widget inside the button, if the button contains a label widget"),
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
                                   PROP_USE_STOCK,
                                   g_param_spec_boolean ("use-stock",
							 P_("Use stock"),
							 P_("If set, the label is used to pick a stock item instead of being displayed"),
                                                        FALSE,
                                                        BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (bobject_class,
                                   PROP_FOCUS_ON_CLICK,
                                   g_param_spec_boolean ("focus-on-click",
							 P_("Focus on click"),
							 P_("Whether the button grabs focus when it is clicked with the mouse"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (bobject_class,
                                   PROP_RELIEF,
                                   g_param_spec_enum ("relief",
                                                      P_("Border relief"),
                                                      P_("The border relief style"),
                                                      BTK_TYPE_RELIEF_STYLE,
                                                      BTK_RELIEF_NORMAL,
                                                      BTK_PARAM_READWRITE));
  
  /**
   * BtkButton:xalign:
   *
   * If the child of the button is a #BtkMisc or #BtkAlignment, this property 
   * can be used to control it's horizontal alignment. 0.0 is left aligned, 
   * 1.0 is right aligned.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_XALIGN,
                                   g_param_spec_float("xalign",
                                                      P_("Horizontal alignment for child"),
                                                      P_("Horizontal position of child in available space. 0.0 is left aligned, 1.0 is right aligned"),
                                                      0.0,
                                                      1.0,
                                                      0.5,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkButton:yalign:
   *
   * If the child of the button is a #BtkMisc or #BtkAlignment, this property 
   * can be used to control it's vertical alignment. 0.0 is top aligned, 
   * 1.0 is bottom aligned.
   * 
   * Since: 2.4
   */
  g_object_class_install_property (bobject_class,
                                   PROP_YALIGN,
                                   g_param_spec_float("yalign",
                                                      P_("Vertical alignment for child"),
                                                      P_("Vertical position of child in available space. 0.0 is top aligned, 1.0 is bottom aligned"),
                                                      0.0,
                                                      1.0,
                                                      0.5,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkButton::image:
   * 
   * The child widget to appear next to the button text.
   * 
   * Since: 2.6
   */
  g_object_class_install_property (bobject_class,
                                   PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        P_("Image widget"),
                                                        P_("Child widget to appear next to the button text"),
                                                        BTK_TYPE_WIDGET,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkButton:image-position:
   *
   * The position of the image relative to the text inside the button.
   * 
   * Since: 2.10
   */
  g_object_class_install_property (bobject_class,
                                   PROP_IMAGE_POSITION,
                                   g_param_spec_enum ("image-position",
                                            P_("Image position"),
                                                      P_("The position of the image relative to the text"),
                                                      BTK_TYPE_POSITION_TYPE,
                                                      BTK_POS_LEFT,
                                                      BTK_PARAM_READWRITE));

  g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_RELATED_ACTION, "related-action");
  g_object_class_override_property (bobject_class, PROP_ACTIVATABLE_USE_ACTION_APPEARANCE, "use-action-appearance");

  /**
   * BtkButton::pressed:
   * @button: the object that received the signal
   *
   * Emitted when the button is pressed.
   * 
   * Deprecated: 2.8: Use the #BtkWidget::button-press-event signal.
   */ 
  button_signals[PRESSED] =
    g_signal_new (I_("pressed"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkButtonClass, pressed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkButton::released:
   * @button: the object that received the signal
   *
   * Emitted when the button is released.
   * 
   * Deprecated: 2.8: Use the #BtkWidget::button-release-event signal.
   */ 
  button_signals[RELEASED] =
    g_signal_new (I_("released"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkButtonClass, released),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkButton::clicked:
   * @button: the object that received the signal
   *
   * Emitted when the button has been activated (pressed and released).
   */ 
  button_signals[CLICKED] =
    g_signal_new (I_("clicked"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkButtonClass, clicked),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkButton::enter:
   * @button: the object that received the signal
   *
   * Emitted when the pointer enters the button.
   * 
   * Deprecated: 2.8: Use the #BtkWidget::enter-notify-event signal.
   */ 
  button_signals[ENTER] =
    g_signal_new (I_("enter"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkButtonClass, enter),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkButton::leave:
   * @button: the object that received the signal
   *
   * Emitted when the pointer leaves the button.
   * 
   * Deprecated: 2.8: Use the #BtkWidget::leave-notify-event signal.
   */ 
  button_signals[LEAVE] =
    g_signal_new (I_("leave"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkButtonClass, leave),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkButton::activate:
   * @widget: the object which received the signal.
   *
   * The ::activate signal on BtkButton is an action signal and
   * emitting it causes the button to animate press then release. 
   * Applications should never connect to this signal, but use the
   * #BtkButton::clicked signal.
   */
  button_signals[ACTIVATE] =
    g_signal_new (I_("activate"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (BtkButtonClass, activate),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);
  widget_class->activate_signal = button_signals[ACTIVATE];

  /**
   * BtkButton:default-border:
   *
   * The "default-border" style property defines the extra space to add
   * around a button that can become the default widget of its window.
   * For more information about default widgets, see btk_widget_grab_default().
   */

  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("default-border",
							       P_("Default Spacing"),
							       P_("Extra space to add for BTK_CAN_DEFAULT buttons"),
							       BTK_TYPE_BORDER,
							       BTK_PARAM_READABLE));

  /**
   * BtkButton:default-outside-border:
   *
   * The "default-outside-border" style property defines the extra outside
   * space to add around a button that can become the default widget of its
   * window. Extra outside space is always drawn outside the button border.
   * For more information about default widgets, see btk_widget_grab_default().
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("default-outside-border",
							       P_("Default Outside Spacing"),
							       P_("Extra space to add for BTK_CAN_DEFAULT buttons that is always drawn outside the border"),
							       BTK_TYPE_BORDER,
							       BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-displacement-x",
							     P_("Child X Displacement"),
							     P_("How far in the x direction to move the child when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     BTK_PARAM_READABLE));
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("child-displacement-y",
							     P_("Child Y Displacement"),
							     P_("How far in the y direction to move the child when the button is depressed"),
							     G_MININT,
							     G_MAXINT,
							     0,
							     BTK_PARAM_READABLE));

  /**
   * BtkButton:displace-focus:
   *
   * Whether the child_displacement_x/child_displacement_y properties 
   * should also affect the focus rectangle.
   *
   * Since: 2.6
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boolean ("displace-focus",
								 P_("Displace focus"),
								 P_("Whether the child_displacement_x/_y properties should also affect the focus rectangle"),
								 FALSE,
								 BTK_PARAM_READABLE));

  /**
   * BtkButton:inner-border:
   *
   * Sets the border between the button edges and child.
   *
   * Since: 2.10
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_boxed ("inner-border",
                                                               P_("Inner Border"),
                                                               P_("Border between button edges and child."),
                                                               BTK_TYPE_BORDER,
                                                               BTK_PARAM_READABLE));

  /**
   * BtkButton::image-spacing:
   * 
   * Spacing in pixels between the image and label.
   * 
   * Since: 2.10
   */
  btk_widget_class_install_style_property (widget_class,
					   g_param_spec_int ("image-spacing",
							     P_("Image spacing"),
							     P_("Spacing in pixels between the image and label"),
							     0,
							     G_MAXINT,
							     2,
							     BTK_PARAM_READABLE));

  g_type_class_add_private (bobject_class, sizeof (BtkButtonPrivate));
}

static void
btk_button_init (BtkButton *button)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  btk_widget_set_can_focus (BTK_WIDGET (button), TRUE);
  btk_widget_set_receives_default (BTK_WIDGET (button), TRUE);
  btk_widget_set_has_window (BTK_WIDGET (button), FALSE);

  button->label_text = NULL;
  
  button->constructed = FALSE;
  button->in_button = FALSE;
  button->button_down = FALSE;
  button->relief = BTK_RELIEF_NORMAL;
  button->use_stock = FALSE;
  button->use_underline = FALSE;
  button->depressed = FALSE;
  button->depress_on_activate = TRUE;
  button->focus_on_click = TRUE;

  priv->xalign = 0.5;
  priv->yalign = 0.5;
  priv->align_set = 0;
  priv->image_is_stock = TRUE;
  priv->image_position = BTK_POS_LEFT;
  priv->use_action_appearance = TRUE;
}

static void
btk_button_destroy (BtkObject *object)
{
  BtkButton *button = BTK_BUTTON (object);
  
  if (button->label_text)
    {
      g_free (button->label_text);
      button->label_text = NULL;
    }

  BTK_OBJECT_CLASS (btk_button_parent_class)->destroy (object);
}

static BObject*
btk_button_constructor (GType                  type,
			guint                  n_construct_properties,
			BObjectConstructParam *construct_params)
{
  BObject *object;
  BtkButton *button;

  object = B_OBJECT_CLASS (btk_button_parent_class)->constructor (type,
                                                                  n_construct_properties,
                                                                  construct_params);

  button = BTK_BUTTON (object);
  button->constructed = TRUE;

  if (button->label_text != NULL)
    btk_button_construct_child (button);
  
  return object;
}


static GType
btk_button_child_type  (BtkContainer     *container)
{
  if (!BTK_BIN (container)->child)
    return BTK_TYPE_WIDGET;
  else
    return B_TYPE_NONE;
}

static void
maybe_set_alignment (BtkButton *button,
		     BtkWidget *widget)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  if (BTK_IS_MISC (widget))
    {
      BtkMisc *misc = BTK_MISC (widget);
      
      if (priv->align_set)
	btk_misc_set_alignment (misc, priv->xalign, priv->yalign);
    }
  else if (BTK_IS_ALIGNMENT (widget))
    {
      BtkAlignment *alignment = BTK_ALIGNMENT (widget);

      if (priv->align_set)
	btk_alignment_set (alignment, priv->xalign, priv->yalign, 
			   alignment->xscale, alignment->yscale);
    }
}

static void
btk_button_add (BtkContainer *container,
		BtkWidget    *widget)
{
  maybe_set_alignment (BTK_BUTTON (container), widget);

  BTK_CONTAINER_CLASS (btk_button_parent_class)->add (container, widget);
}

static void 
btk_button_dispose (BObject *object)
{
  BtkButton *button = BTK_BUTTON (object);
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->action)
    {
      btk_activatable_do_set_related_action (BTK_ACTIVATABLE (button), NULL);
      priv->action = NULL;
    }
  B_OBJECT_CLASS (btk_button_parent_class)->dispose (object);
}

static void
btk_button_set_property (BObject         *object,
                         guint            prop_id,
                         const BValue    *value,
                         BParamSpec      *pspec)
{
  BtkButton *button = BTK_BUTTON (object);
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  switch (prop_id)
    {
    case PROP_LABEL:
      btk_button_set_label (button, b_value_get_string (value));
      break;
    case PROP_IMAGE:
      btk_button_set_image (button, (BtkWidget *) b_value_get_object (value));
      break;
    case PROP_RELIEF:
      btk_button_set_relief (button, b_value_get_enum (value));
      break;
    case PROP_USE_UNDERLINE:
      btk_button_set_use_underline (button, b_value_get_boolean (value));
      break;
    case PROP_USE_STOCK:
      btk_button_set_use_stock (button, b_value_get_boolean (value));
      break;
    case PROP_FOCUS_ON_CLICK:
      btk_button_set_focus_on_click (button, b_value_get_boolean (value));
      break;
    case PROP_XALIGN:
      btk_button_set_alignment (button, b_value_get_float (value), priv->yalign);
      break;
    case PROP_YALIGN:
      btk_button_set_alignment (button, priv->xalign, b_value_get_float (value));
      break;
    case PROP_IMAGE_POSITION:
      btk_button_set_image_position (button, b_value_get_enum (value));
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      btk_button_set_related_action (button, b_value_get_object (value));
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      btk_button_set_use_action_appearance (button, b_value_get_boolean (value));
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_button_get_property (BObject         *object,
                         guint            prop_id,
                         BValue          *value,
                         BParamSpec      *pspec)
{
  BtkButton *button = BTK_BUTTON (object);
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  switch (prop_id)
    {
    case PROP_LABEL:
      b_value_set_string (value, button->label_text);
      break;
    case PROP_IMAGE:
      b_value_set_object (value, (BObject *)priv->image);
      break;
    case PROP_RELIEF:
      b_value_set_enum (value, btk_button_get_relief (button));
      break;
    case PROP_USE_UNDERLINE:
      b_value_set_boolean (value, button->use_underline);
      break;
    case PROP_USE_STOCK:
      b_value_set_boolean (value, button->use_stock);
      break;
    case PROP_FOCUS_ON_CLICK:
      b_value_set_boolean (value, button->focus_on_click);
      break;
    case PROP_XALIGN:
      b_value_set_float (value, priv->xalign);
      break;
    case PROP_YALIGN:
      b_value_set_float (value, priv->yalign);
      break;
    case PROP_IMAGE_POSITION:
      b_value_set_enum (value, priv->image_position);
      break;
    case PROP_ACTIVATABLE_RELATED_ACTION:
      b_value_set_object (value, priv->action);
      break;
    case PROP_ACTIVATABLE_USE_ACTION_APPEARANCE:
      b_value_set_boolean (value, priv->use_action_appearance);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void 
btk_button_activatable_interface_init (BtkActivatableIface  *iface)
{
  iface->update = btk_button_update;
  iface->sync_action_properties = btk_button_sync_action_properties;
}

static void
activatable_update_stock_id (BtkButton *button,
			     BtkAction *action)
{
  if (!btk_button_get_use_stock (button))
    return;

  btk_button_set_label (button, btk_action_get_stock_id (action));
}

static void
activatable_update_short_label (BtkButton *button,
				BtkAction *action)
{
  BtkWidget *image;

  if (btk_button_get_use_stock (button))
    return;

  image = btk_button_get_image (button);

  /* Dont touch custom child... */
  if (BTK_IS_IMAGE (image) ||
      BTK_BIN (button)->child == NULL || 
      BTK_IS_LABEL (BTK_BIN (button)->child))
    {
      btk_button_set_label (button, btk_action_get_short_label (action));
      btk_button_set_use_underline (button, TRUE);
    }
}

static void
activatable_update_icon_name (BtkButton *button,
			      BtkAction *action)
{
  BtkWidget *image;
	      
  if (btk_button_get_use_stock (button))
    return;

  image = btk_button_get_image (button);

  if (BTK_IS_IMAGE (image) &&
      (btk_image_get_storage_type (BTK_IMAGE (image)) == BTK_IMAGE_EMPTY ||
       btk_image_get_storage_type (BTK_IMAGE (image)) == BTK_IMAGE_ICON_NAME))
    btk_image_set_from_icon_name (BTK_IMAGE (image),
				  btk_action_get_icon_name (action), BTK_ICON_SIZE_MENU);
}

static void
activatable_update_gicon (BtkButton *button,
			  BtkAction *action)
{
  BtkWidget *image = btk_button_get_image (button);
  GIcon *icon = btk_action_get_gicon (action);
  
  if (BTK_IS_IMAGE (image) &&
      (btk_image_get_storage_type (BTK_IMAGE (image)) == BTK_IMAGE_EMPTY ||
       btk_image_get_storage_type (BTK_IMAGE (image)) == BTK_IMAGE_GICON))
    btk_image_set_from_gicon (BTK_IMAGE (image), icon, BTK_ICON_SIZE_BUTTON);
}

static void 
btk_button_update (BtkActivatable *activatable,
		   BtkAction      *action,
	           const gchar    *property_name)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (activatable);

  if (strcmp (property_name, "visible") == 0)
    {
      if (btk_action_is_visible (action))
	btk_widget_show (BTK_WIDGET (activatable));
      else
	btk_widget_hide (BTK_WIDGET (activatable));
    }
  else if (strcmp (property_name, "sensitive") == 0)
    btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));

  if (!priv->use_action_appearance)
    return;

  if (strcmp (property_name, "stock-id") == 0)
    activatable_update_stock_id (BTK_BUTTON (activatable), action);
  else if (strcmp (property_name, "gicon") == 0)
    activatable_update_gicon (BTK_BUTTON (activatable), action);
  else if (strcmp (property_name, "short-label") == 0)
    activatable_update_short_label (BTK_BUTTON (activatable), action);
  else if (strcmp (property_name, "icon-name") == 0)
    activatable_update_icon_name (BTK_BUTTON (activatable), action);
}

static void
btk_button_sync_action_properties (BtkActivatable *activatable,
			           BtkAction      *action)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (activatable);

  if (!action)
    return;

  if (btk_action_is_visible (action))
    btk_widget_show (BTK_WIDGET (activatable));
  else
    btk_widget_hide (BTK_WIDGET (activatable));
  
  btk_widget_set_sensitive (BTK_WIDGET (activatable), btk_action_is_sensitive (action));
  
  if (priv->use_action_appearance)
    {
      activatable_update_stock_id (BTK_BUTTON (activatable), action);
      activatable_update_short_label (BTK_BUTTON (activatable), action);
      activatable_update_gicon (BTK_BUTTON (activatable), action);
      activatable_update_icon_name (BTK_BUTTON (activatable), action);
    }
}

static void
btk_button_set_related_action (BtkButton *button,
			       BtkAction *action)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->action == action)
    return;

  /* This should be a default handler, but for compatibility reasons
   * we need to support derived classes that don't chain up their
   * clicked handler.
   */
  g_signal_handlers_disconnect_by_func (button, btk_real_button_clicked, NULL);
  if (action)
    g_signal_connect_after (button, "clicked",
                            G_CALLBACK (btk_real_button_clicked), NULL);

  btk_activatable_do_set_related_action (BTK_ACTIVATABLE (button), action);

  priv->action = action;
}

static void
btk_button_set_use_action_appearance (BtkButton *button,
				      gboolean   use_appearance)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->use_action_appearance != use_appearance)
    {
      priv->use_action_appearance = use_appearance;

      btk_activatable_sync_action_properties (BTK_ACTIVATABLE (button), priv->action);
    }
}

BtkWidget*
btk_button_new (void)
{
  return g_object_new (BTK_TYPE_BUTTON, NULL);
}

static gboolean
show_image (BtkButton *button)
{
  gboolean show;
  
  if (button->label_text)
    {
      BtkSettings *settings;

      settings = btk_widget_get_settings (BTK_WIDGET (button));        
      g_object_get (settings, "btk-button-images", &show, NULL);
    }
  else
    show = TRUE;

  return show;
}

static void
btk_button_construct_child (BtkButton *button)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);
  BtkStockItem item;
  BtkWidget *label;
  BtkWidget *box;
  BtkWidget *align;
  BtkWidget *image = NULL;
  gchar *label_text = NULL;
  gint image_spacing;

  if (!button->constructed)
    return;

  if (!button->label_text && !priv->image)
    return;

  btk_widget_style_get (BTK_WIDGET (button),
			"image-spacing", &image_spacing,
			NULL);

  if (priv->image && !priv->image_is_stock)
    {
      image = g_object_ref (priv->image);
      if (image->parent)
	btk_container_remove (BTK_CONTAINER (image->parent), image);
    }

  priv->image = NULL;

  if (BTK_BIN (button)->child)
    btk_container_remove (BTK_CONTAINER (button),
			  BTK_BIN (button)->child);

  if (button->use_stock &&
      button->label_text &&
      btk_stock_lookup (button->label_text, &item))
    {
      if (!image)
	image = g_object_ref (btk_image_new_from_stock (button->label_text, BTK_ICON_SIZE_BUTTON));

      label_text = item.label;
    }
  else
    label_text = button->label_text;

  if (image)
    {
      priv->image = image;
      g_object_set (priv->image,
		    "visible", show_image (button),
		    "no-show-all", TRUE,
		    NULL);

      if (priv->image_position == BTK_POS_LEFT ||
	  priv->image_position == BTK_POS_RIGHT)
	box = btk_hbox_new (FALSE, image_spacing);
      else
	box = btk_vbox_new (FALSE, image_spacing);

      if (priv->align_set)
	align = btk_alignment_new (priv->xalign, priv->yalign, 0.0, 0.0);
      else
	align = btk_alignment_new (0.5, 0.5, 0.0, 0.0);

      if (priv->image_position == BTK_POS_LEFT ||
	  priv->image_position == BTK_POS_TOP)
	btk_box_pack_start (BTK_BOX (box), priv->image, FALSE, FALSE, 0);
      else
	btk_box_pack_end (BTK_BOX (box), priv->image, FALSE, FALSE, 0);

      if (label_text)
	{
          if (button->use_underline || button->use_stock)
            {
	      label = btk_label_new_with_mnemonic (label_text);
	      btk_label_set_mnemonic_widget (BTK_LABEL (label),
                                             BTK_WIDGET (button));
            }
          else
            label = btk_label_new (label_text);

	  if (priv->image_position == BTK_POS_RIGHT ||
	      priv->image_position == BTK_POS_BOTTOM)
	    btk_box_pack_start (BTK_BOX (box), label, FALSE, FALSE, 0);
	  else
	    btk_box_pack_end (BTK_BOX (box), label, FALSE, FALSE, 0);
	}

      btk_container_add (BTK_CONTAINER (button), align);
      btk_container_add (BTK_CONTAINER (align), box);
      btk_widget_show_all (align);

      g_object_unref (image);

      return;
    }

  if (button->use_underline || button->use_stock)
    {
      label = btk_label_new_with_mnemonic (button->label_text);
      btk_label_set_mnemonic_widget (BTK_LABEL (label), BTK_WIDGET (button));
    }
  else
    label = btk_label_new (button->label_text);

  if (priv->align_set)
    btk_misc_set_alignment (BTK_MISC (label), priv->xalign, priv->yalign);

  btk_widget_show (label);
  btk_container_add (BTK_CONTAINER (button), label);
}


BtkWidget*
btk_button_new_with_label (const gchar *label)
{
  return g_object_new (BTK_TYPE_BUTTON, "label", label, NULL);
}

/**
 * btk_button_new_from_stock:
 * @stock_id: the name of the stock item 
 *
 * Creates a new #BtkButton containing the image and text from a stock item.
 * Some stock ids have preprocessor macros like #BTK_STOCK_OK and
 * #BTK_STOCK_APPLY.
 *
 * If @stock_id is unknown, then it will be treated as a mnemonic
 * label (as for btk_button_new_with_mnemonic()).
 *
 * Returns: a new #BtkButton
 **/
BtkWidget*
btk_button_new_from_stock (const gchar *stock_id)
{
  return g_object_new (BTK_TYPE_BUTTON,
                       "label", stock_id,
                       "use-stock", TRUE,
                       "use-underline", TRUE,
                       NULL);
}

/**
 * btk_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *         mnemonic character
 * @returns: a new #BtkButton
 *
 * Creates a new #BtkButton containing a label.
 * If characters in @label are preceded by an underscore, they are underlined.
 * If you need a literal underscore character in a label, use '__' (two 
 * underscores). The first underlined character represents a keyboard 
 * accelerator called a mnemonic.
 * Pressing Alt and that key activates the button.
 **/
BtkWidget*
btk_button_new_with_mnemonic (const gchar *label)
{
  return g_object_new (BTK_TYPE_BUTTON, "label", label, "use-underline", TRUE,  NULL);
}

void
btk_button_pressed (BtkButton *button)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  
  g_signal_emit (button, button_signals[PRESSED], 0);
}

void
btk_button_released (BtkButton *button)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[RELEASED], 0);
}

void
btk_button_clicked (BtkButton *button)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[CLICKED], 0);
}

void
btk_button_enter (BtkButton *button)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[ENTER], 0);
}

void
btk_button_leave (BtkButton *button)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  g_signal_emit (button, button_signals[LEAVE], 0);
}

void
btk_button_set_relief (BtkButton *button,
		       BtkReliefStyle newrelief)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  if (newrelief != button->relief) 
    {
       button->relief = newrelief;
       g_object_notify (B_OBJECT (button), "relief");
       btk_widget_queue_draw (BTK_WIDGET (button));
    }
}

BtkReliefStyle
btk_button_get_relief (BtkButton *button)
{
  g_return_val_if_fail (BTK_IS_BUTTON (button), BTK_RELIEF_NORMAL);

  return button->relief;
}

static void
btk_button_realize (BtkWidget *widget)
{
  BtkButton *button;
  BdkWindowAttr attributes;
  gint attributes_mask;
  gint border_width;

  button = BTK_BUTTON (widget);
  btk_widget_set_realized (widget, TRUE);

  border_width = BTK_CONTAINER (widget)->border_width;

  attributes.window_type = BDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x + border_width;
  attributes.y = widget->allocation.y + border_width;
  attributes.width = widget->allocation.width - border_width * 2;
  attributes.height = widget->allocation.height - border_width * 2;
  attributes.wclass = BDK_INPUT_ONLY;
  attributes.event_mask = btk_widget_get_events (widget);
  attributes.event_mask |= (BDK_BUTTON_PRESS_MASK |
			    BDK_BUTTON_RELEASE_MASK |
			    BDK_ENTER_NOTIFY_MASK |
			    BDK_LEAVE_NOTIFY_MASK);

  attributes_mask = BDK_WA_X | BDK_WA_Y;

  widget->window = btk_widget_get_parent_window (widget);
  g_object_ref (widget->window);
  
  button->event_window = bdk_window_new (btk_widget_get_parent_window (widget),
					 &attributes, attributes_mask);
  bdk_window_set_user_data (button->event_window, button);

  widget->style = btk_style_attach (widget->style, widget->window);
}

static void
btk_button_unrealize (BtkWidget *widget)
{
  BtkButton *button = BTK_BUTTON (widget);

  if (button->activate_timeout)
    btk_button_finish_activate (button, FALSE);

  if (button->event_window)
    {
      bdk_window_set_user_data (button->event_window, NULL);
      bdk_window_destroy (button->event_window);
      button->event_window = NULL;
    }
  
  BTK_WIDGET_CLASS (btk_button_parent_class)->unrealize (widget);
}

static void
btk_button_map (BtkWidget *widget)
{
  BtkButton *button = BTK_BUTTON (widget);
  
  BTK_WIDGET_CLASS (btk_button_parent_class)->map (widget);

  if (button->event_window)
    bdk_window_show (button->event_window);
}

static void
btk_button_unmap (BtkWidget *widget)
{
  BtkButton *button = BTK_BUTTON (widget);
    
  if (button->event_window)
    bdk_window_hide (button->event_window);

  BTK_WIDGET_CLASS (btk_button_parent_class)->unmap (widget);
}

static void
btk_button_update_image_spacing (BtkButton *button)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);
  BtkWidget *child; 
  gint spacing;

  /* Keep in sync with btk_button_construct_child,
   * we only want to update the spacing if the box 
   * was constructed there.
   */
  if (!button->constructed || !priv->image)
    return;

  child = BTK_BIN (button)->child;
  if (BTK_IS_ALIGNMENT (child))
    {
      child = BTK_BIN (child)->child;
      if (BTK_IS_BOX (child))
        {
          btk_widget_style_get (BTK_WIDGET (button),
                                "image-spacing", &spacing,
                                NULL);

          btk_box_set_spacing (BTK_BOX (child), spacing);
        }
    }   
}

static void
btk_button_style_set (BtkWidget *widget,
		      BtkStyle  *prev_style)
{
  btk_button_update_image_spacing (BTK_BUTTON (widget));
}

static void
btk_button_get_props (BtkButton *button,
		      BtkBorder *default_border,
		      BtkBorder *default_outside_border,
                      BtkBorder *inner_border,
		      gboolean  *interior_focus)
{
  BtkWidget *widget =  BTK_WIDGET (button);
  BtkBorder *tmp_border;

  if (default_border)
    {
      btk_widget_style_get (widget, "default-border", &tmp_border, NULL);

      if (tmp_border)
	{
	  *default_border = *tmp_border;
	  btk_border_free (tmp_border);
	}
      else
	*default_border = default_default_border;
    }

  if (default_outside_border)
    {
      btk_widget_style_get (widget, "default-outside-border", &tmp_border, NULL);

      if (tmp_border)
	{
	  *default_outside_border = *tmp_border;
	  btk_border_free (tmp_border);
	}
      else
	*default_outside_border = default_default_outside_border;
    }

  if (inner_border)
    {
      btk_widget_style_get (widget, "inner-border", &tmp_border, NULL);

      if (tmp_border)
	{
	  *inner_border = *tmp_border;
	  btk_border_free (tmp_border);
	}
      else
	*inner_border = default_inner_border;
    }

  if (interior_focus)
    btk_widget_style_get (widget, "interior-focus", interior_focus, NULL);
}
	
static void
btk_button_size_request (BtkWidget      *widget,
			 BtkRequisition *requisition)
{
  BtkButton *button = BTK_BUTTON (widget);
  BtkBorder default_border;
  BtkBorder inner_border;
  gint focus_width;
  gint focus_pad;

  btk_button_get_props (button, &default_border, NULL, &inner_border, NULL);
  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);
 
  requisition->width = ((BTK_CONTAINER (widget)->border_width +
                         BTK_WIDGET (widget)->style->xthickness) * 2 +
                        inner_border.left + inner_border.right);
  requisition->height = ((BTK_CONTAINER (widget)->border_width +
                          BTK_WIDGET (widget)->style->ythickness) * 2 +
                         inner_border.top + inner_border.bottom);

  if (btk_widget_get_can_default (widget))
    {
      requisition->width += default_border.left + default_border.right;
      requisition->height += default_border.top + default_border.bottom;
    }

  if (BTK_BIN (button)->child && btk_widget_get_visible (BTK_BIN (button)->child))
    {
      BtkRequisition child_requisition;

      btk_widget_size_request (BTK_BIN (button)->child, &child_requisition);

      requisition->width += child_requisition.width;
      requisition->height += child_requisition.height;
    }
  
  requisition->width += 2 * (focus_width + focus_pad);
  requisition->height += 2 * (focus_width + focus_pad);
}

static void
btk_button_size_allocate (BtkWidget     *widget,
			  BtkAllocation *allocation)
{
  BtkButton *button = BTK_BUTTON (widget);
  BtkAllocation child_allocation;

  gint border_width = BTK_CONTAINER (widget)->border_width;
  gint xthickness = BTK_WIDGET (widget)->style->xthickness;
  gint ythickness = BTK_WIDGET (widget)->style->ythickness;
  BtkBorder default_border;
  BtkBorder inner_border;
  gint focus_width;
  gint focus_pad;

  btk_button_get_props (button, &default_border, NULL, &inner_border, NULL);
  btk_widget_style_get (BTK_WIDGET (widget),
			"focus-line-width", &focus_width,
			"focus-padding", &focus_pad,
			NULL);
 
			    
  widget->allocation = *allocation;

  if (btk_widget_get_realized (widget))
    bdk_window_move_resize (button->event_window,
			    widget->allocation.x + border_width,
			    widget->allocation.y + border_width,
			    widget->allocation.width - border_width * 2,
			    widget->allocation.height - border_width * 2);

  if (BTK_BIN (button)->child && btk_widget_get_visible (BTK_BIN (button)->child))
    {
      child_allocation.x = widget->allocation.x + border_width + inner_border.left + xthickness;
      child_allocation.y = widget->allocation.y + border_width + inner_border.top + ythickness;
      
      child_allocation.width = MAX (1, widget->allocation.width -
                                    xthickness * 2 -
                                    inner_border.left -
                                    inner_border.right -
				    border_width * 2);
      child_allocation.height = MAX (1, widget->allocation.height -
                                     ythickness * 2 -
                                     inner_border.top -
                                     inner_border.bottom -
				     border_width * 2);

      if (btk_widget_get_can_default (BTK_WIDGET (button)))
	{
	  child_allocation.x += default_border.left;
	  child_allocation.y += default_border.top;
	  child_allocation.width =  MAX (1, child_allocation.width - default_border.left - default_border.right);
	  child_allocation.height = MAX (1, child_allocation.height - default_border.top - default_border.bottom);
	}

      if (btk_widget_get_can_focus (BTK_WIDGET (button)))
	{
	  child_allocation.x += focus_width + focus_pad;
	  child_allocation.y += focus_width + focus_pad;
	  child_allocation.width =  MAX (1, child_allocation.width - (focus_width + focus_pad) * 2);
	  child_allocation.height = MAX (1, child_allocation.height - (focus_width + focus_pad) * 2);
	}

      if (button->depressed)
	{
	  gint child_displacement_x;
	  gint child_displacement_y;
	  
	  btk_widget_style_get (widget,
				"child-displacement-x", &child_displacement_x, 
				"child-displacement-y", &child_displacement_y,
				NULL);
	  child_allocation.x += child_displacement_x;
	  child_allocation.y += child_displacement_y;
	}

      btk_widget_size_allocate (BTK_BIN (button)->child, &child_allocation);
    }
}

void
_btk_button_paint (BtkButton          *button,
		   const BdkRectangle *area,
		   BtkStateType        state_type,
		   BtkShadowType       shadow_type,
		   const gchar        *main_detail,
		   const gchar        *default_detail)
{
  BtkWidget *widget;
  gint width, height;
  gint x, y;
  gint border_width;
  BtkBorder default_border;
  BtkBorder default_outside_border;
  gboolean interior_focus;
  gint focus_width;
  gint focus_pad;

  widget = BTK_WIDGET (button);

  if (btk_widget_is_drawable (widget))
    {
      border_width = BTK_CONTAINER (widget)->border_width;

      btk_button_get_props (button, &default_border, &default_outside_border, NULL, &interior_focus);
      btk_widget_style_get (widget,
			    "focus-line-width", &focus_width,
			    "focus-padding", &focus_pad,
			    NULL); 
	
      x = widget->allocation.x + border_width;
      y = widget->allocation.y + border_width;
      width = widget->allocation.width - border_width * 2;
      height = widget->allocation.height - border_width * 2;

      if (btk_widget_has_default (widget) &&
	  BTK_BUTTON (widget)->relief == BTK_RELIEF_NORMAL)
	{
	  btk_paint_box (widget->style, widget->window,
			 BTK_STATE_NORMAL, BTK_SHADOW_IN,
			 area, widget, "buttondefault",
			 x, y, width, height);

	  x += default_border.left;
	  y += default_border.top;
	  width -= default_border.left + default_border.right;
	  height -= default_border.top + default_border.bottom;
	}
      else if (btk_widget_get_can_default (widget))
	{
	  x += default_outside_border.left;
	  y += default_outside_border.top;
	  width -= default_outside_border.left + default_outside_border.right;
	  height -= default_outside_border.top + default_outside_border.bottom;
	}
       
      if (!interior_focus && btk_widget_has_focus (widget))
	{
	  x += focus_width + focus_pad;
	  y += focus_width + focus_pad;
	  width -= 2 * (focus_width + focus_pad);
	  height -= 2 * (focus_width + focus_pad);
	}

      if (button->relief != BTK_RELIEF_NONE || button->depressed ||
	  btk_widget_get_state(widget) == BTK_STATE_PRELIGHT)
	btk_paint_box (widget->style, widget->window,
		       state_type,
		       shadow_type, area, widget, "button",
		       x, y, width, height);
       
      if (btk_widget_has_focus (widget))
	{
	  gint child_displacement_x;
	  gint child_displacement_y;
	  gboolean displace_focus;
	  
	  btk_widget_style_get (widget,
				"child-displacement-y", &child_displacement_y,
				"child-displacement-x", &child_displacement_x,
				"displace-focus", &displace_focus,
				NULL);

	  if (interior_focus)
	    {
	      x += widget->style->xthickness + focus_pad;
	      y += widget->style->ythickness + focus_pad;
	      width -= 2 * (widget->style->xthickness + focus_pad);
	      height -=  2 * (widget->style->ythickness + focus_pad);
	    }
	  else
	    {
	      x -= focus_width + focus_pad;
	      y -= focus_width + focus_pad;
	      width += 2 * (focus_width + focus_pad);
	      height += 2 * (focus_width + focus_pad);
	    }

	  if (button->depressed && displace_focus)
	    {
	      x += child_displacement_x;
	      y += child_displacement_y;
	    }

	  btk_paint_focus (widget->style, widget->window, btk_widget_get_state (widget),
			   area, widget, "button",
			   x, y, width, height);
	}
    }
}

static gboolean
btk_button_expose (BtkWidget      *widget,
		   BdkEventExpose *event)
{
  if (btk_widget_is_drawable (widget))
    {
      BtkButton *button = BTK_BUTTON (widget);
      
      _btk_button_paint (button, &event->area,
			 btk_widget_get_state (widget),
			 button->depressed ? BTK_SHADOW_IN : BTK_SHADOW_OUT,
			 "button", "buttondefault");

      BTK_WIDGET_CLASS (btk_button_parent_class)->expose_event (widget, event);
    }

  return FALSE;
}

static gboolean
btk_button_button_press (BtkWidget      *widget,
			 BdkEventButton *event)
{
  BtkButton *button;

  if (event->type == BDK_BUTTON_PRESS)
    {
      button = BTK_BUTTON (widget);

      if (button->focus_on_click && !btk_widget_has_focus (widget))
	btk_widget_grab_focus (widget);

      if (event->button == 1)
	btk_button_pressed (button);
    }

  return TRUE;
}

static gboolean
btk_button_button_release (BtkWidget      *widget,
			   BdkEventButton *event)
{
  BtkButton *button;

  if (event->button == 1)
    {
      button = BTK_BUTTON (widget);
      btk_button_released (button);
    }

  return TRUE;
}

static gboolean
btk_button_grab_broken (BtkWidget          *widget,
			BdkEventGrabBroken *event)
{
  BtkButton *button = BTK_BUTTON (widget);
  gboolean save_in;
  
  /* Simulate a button release without the pointer in the button */
  if (button->button_down)
    {
      save_in = button->in_button;
      button->in_button = FALSE;
      btk_button_released (button);
      if (save_in != button->in_button)
	{
	  button->in_button = save_in;
	  btk_button_update_state (button);
	}
    }

  return TRUE;
}

static gboolean
btk_button_key_release (BtkWidget   *widget,
			BdkEventKey *event)
{
  BtkButton *button = BTK_BUTTON (widget);

  if (button->activate_timeout)
    {
      btk_button_finish_activate (button, TRUE);
      return TRUE;
    }
  else if (BTK_WIDGET_CLASS (btk_button_parent_class)->key_release_event)
    return BTK_WIDGET_CLASS (btk_button_parent_class)->key_release_event (widget, event);
  else
    return FALSE;
}

static gboolean
btk_button_enter_notify (BtkWidget        *widget,
			 BdkEventCrossing *event)
{
  BtkButton *button;
  BtkWidget *event_widget;

  button = BTK_BUTTON (widget);
  event_widget = btk_get_event_widget ((BdkEvent*) event);

  if ((event_widget == widget) &&
      (event->detail != BDK_NOTIFY_INFERIOR))
    {
      button->in_button = TRUE;
      btk_button_enter (button);
    }

  return FALSE;
}

static gboolean
btk_button_leave_notify (BtkWidget        *widget,
			 BdkEventCrossing *event)
{
  BtkButton *button;
  BtkWidget *event_widget;

  button = BTK_BUTTON (widget);
  event_widget = btk_get_event_widget ((BdkEvent*) event);

  if ((event_widget == widget) &&
      (event->detail != BDK_NOTIFY_INFERIOR) &&
      (btk_widget_get_sensitive (event_widget)))
    {
      button->in_button = FALSE;
      btk_button_leave (button);
    }

  return FALSE;
}

static void
btk_real_button_pressed (BtkButton *button)
{
  if (button->activate_timeout)
    return;
  
  button->button_down = TRUE;
  btk_button_update_state (button);
}

static void
btk_real_button_released (BtkButton *button)
{
  if (button->button_down)
    {
      button->button_down = FALSE;

      if (button->activate_timeout)
	return;
      
      if (button->in_button)
	btk_button_clicked (button);

      btk_button_update_state (button);
    }
}

static void 
btk_real_button_clicked (BtkButton *button)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->action)
    btk_action_activate (priv->action);
}

static gboolean
button_activate_timeout (gpointer data)
{
  btk_button_finish_activate (data, TRUE);

  return FALSE;
}

static void
btk_real_button_activate (BtkButton *button)
{
  BtkWidget *widget = BTK_WIDGET (button);
  BtkButtonPrivate *priv;
  guint32 time;

  priv = BTK_BUTTON_GET_PRIVATE (button);

  if (btk_widget_get_realized (widget) && !button->activate_timeout)
    {
      time = btk_get_current_event_time ();
      if (bdk_keyboard_grab (button->event_window, TRUE, time) == 
	  BDK_GRAB_SUCCESS)
	{
	  priv->has_grab = TRUE;
	  priv->grab_time = time;
	}

      btk_grab_add (widget);
      
      button->activate_timeout = bdk_threads_add_timeout (ACTIVATE_TIMEOUT,
						button_activate_timeout,
						button);
      button->button_down = TRUE;
      btk_button_update_state (button);
      btk_widget_queue_draw (BTK_WIDGET (button));
    }
}

static void
btk_button_finish_activate (BtkButton *button,
			    gboolean   do_it)
{
  BtkWidget *widget = BTK_WIDGET (button);
  BtkButtonPrivate *priv;
  
  priv = BTK_BUTTON_GET_PRIVATE (button);

  g_source_remove (button->activate_timeout);
  button->activate_timeout = 0;

  if (priv->has_grab)
    {
      bdk_display_keyboard_ungrab (btk_widget_get_display (widget),
				   priv->grab_time);
    }
  btk_grab_remove (widget);

  button->button_down = FALSE;

  btk_button_update_state (button);
  btk_widget_queue_draw (BTK_WIDGET (button));

  if (do_it)
    btk_button_clicked (button);
}

/**
 * btk_button_set_label:
 * @button: a #BtkButton
 * @label: a string
 *
 * Sets the text of the label of the button to @str. This text is
 * also used to select the stock item if btk_button_set_use_stock()
 * is used.
 *
 * This will also clear any previously set labels.
 **/
void
btk_button_set_label (BtkButton   *button,
		      const gchar *label)
{
  gchar *new_label;
  
  g_return_if_fail (BTK_IS_BUTTON (button));

  new_label = g_strdup (label);
  g_free (button->label_text);
  button->label_text = new_label;
  
  btk_button_construct_child (button);
  
  g_object_notify (B_OBJECT (button), "label");
}

/**
 * btk_button_get_label:
 * @button: a #BtkButton
 *
 * Fetches the text from the label of the button, as set by
 * btk_button_set_label(). If the label text has not 
 * been set the return value will be %NULL. This will be the 
 * case if you create an empty button with btk_button_new() to 
 * use as a container.
 *
 * Return value: The text of the label widget. This string is owned
 * by the widget and must not be modified or freed.
 **/
const gchar *
btk_button_get_label (BtkButton *button)
{
  g_return_val_if_fail (BTK_IS_BUTTON (button), NULL);
  
  return button->label_text;
}

/**
 * btk_button_set_use_underline:
 * @button: a #BtkButton
 * @use_underline: %TRUE if underlines in the text indicate mnemonics
 *
 * If true, an underline in the text of the button label indicates
 * the next character should be used for the mnemonic accelerator key.
 */
void
btk_button_set_use_underline (BtkButton *button,
			      gboolean   use_underline)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  use_underline = use_underline != FALSE;

  if (use_underline != button->use_underline)
    {
      button->use_underline = use_underline;
  
      btk_button_construct_child (button);
      
      g_object_notify (B_OBJECT (button), "use-underline");
    }
}

/**
 * btk_button_get_use_underline:
 * @button: a #BtkButton
 *
 * Returns whether an embedded underline in the button label indicates a
 * mnemonic. See btk_button_set_use_underline ().
 *
 * Return value: %TRUE if an embedded underline in the button label
 *               indicates the mnemonic accelerator keys.
 **/
gboolean
btk_button_get_use_underline (BtkButton *button)
{
  g_return_val_if_fail (BTK_IS_BUTTON (button), FALSE);
  
  return button->use_underline;
}

/**
 * btk_button_set_use_stock:
 * @button: a #BtkButton
 * @use_stock: %TRUE if the button should use a stock item
 *
 * If %TRUE, the label set on the button is used as a
 * stock id to select the stock item for the button.
 */
void
btk_button_set_use_stock (BtkButton *button,
			  gboolean   use_stock)
{
  g_return_if_fail (BTK_IS_BUTTON (button));

  use_stock = use_stock != FALSE;

  if (use_stock != button->use_stock)
    {
      button->use_stock = use_stock;
  
      btk_button_construct_child (button);
      
      g_object_notify (B_OBJECT (button), "use-stock");
    }
}

/**
 * btk_button_get_use_stock:
 * @button: a #BtkButton
 *
 * Returns whether the button label is a stock item.
 *
 * Return value: %TRUE if the button label is used to
 *               select a stock item instead of being
 *               used directly as the label text.
 */
gboolean
btk_button_get_use_stock (BtkButton *button)
{
  g_return_val_if_fail (BTK_IS_BUTTON (button), FALSE);
  
  return button->use_stock;
}

/**
 * btk_button_set_focus_on_click:
 * @button: a #BtkButton
 * @focus_on_click: whether the button grabs focus when clicked with the mouse
 * 
 * Sets whether the button will grab focus when it is clicked with the mouse.
 * Making mouse clicks not grab focus is useful in places like toolbars where
 * you don't want the keyboard focus removed from the main area of the
 * application.
 *
 * Since: 2.4
 **/
void
btk_button_set_focus_on_click (BtkButton *button,
			       gboolean   focus_on_click)
{
  g_return_if_fail (BTK_IS_BUTTON (button));
  
  focus_on_click = focus_on_click != FALSE;

  if (button->focus_on_click != focus_on_click)
    {
      button->focus_on_click = focus_on_click;
      
      g_object_notify (B_OBJECT (button), "focus-on-click");
    }
}

/**
 * btk_button_get_focus_on_click:
 * @button: a #BtkButton
 * 
 * Returns whether the button grabs focus when it is clicked with the mouse.
 * See btk_button_set_focus_on_click().
 *
 * Return value: %TRUE if the button grabs focus when it is clicked with
 *               the mouse.
 *
 * Since: 2.4
 **/
gboolean
btk_button_get_focus_on_click (BtkButton *button)
{
  g_return_val_if_fail (BTK_IS_BUTTON (button), FALSE);
  
  return button->focus_on_click;
}

/**
 * btk_button_set_alignment:
 * @button: a #BtkButton
 * @xalign: the horizontal position of the child, 0.0 is left aligned, 
 *   1.0 is right aligned
 * @yalign: the vertical position of the child, 0.0 is top aligned, 
 *   1.0 is bottom aligned
 *
 * Sets the alignment of the child. This property has no effect unless 
 * the child is a #BtkMisc or a #BtkAligment.
 *
 * Since: 2.4
 */
void
btk_button_set_alignment (BtkButton *button,
			  gfloat     xalign,
			  gfloat     yalign)
{
  BtkButtonPrivate *priv;

  g_return_if_fail (BTK_IS_BUTTON (button));
  
  priv = BTK_BUTTON_GET_PRIVATE (button);

  priv->xalign = xalign;
  priv->yalign = yalign;
  priv->align_set = 1;

  maybe_set_alignment (button, BTK_BIN (button)->child);

  g_object_freeze_notify (B_OBJECT (button));
  g_object_notify (B_OBJECT (button), "xalign");
  g_object_notify (B_OBJECT (button), "yalign");
  g_object_thaw_notify (B_OBJECT (button));
}

/**
 * btk_button_get_alignment:
 * @button: a #BtkButton
 * @xalign: (out): return location for horizontal alignment
 * @yalign: (out): return location for vertical alignment
 *
 * Gets the alignment of the child in the button.
 *
 * Since: 2.4
 */
void
btk_button_get_alignment (BtkButton *button,
			  gfloat    *xalign,
			  gfloat    *yalign)
{
  BtkButtonPrivate *priv;

  g_return_if_fail (BTK_IS_BUTTON (button));
  
  priv = BTK_BUTTON_GET_PRIVATE (button);
 
  if (xalign) 
    *xalign = priv->xalign;

  if (yalign)
    *yalign = priv->yalign;
}

/**
 * _btk_button_set_depressed:
 * @button: a #BtkButton
 * @depressed: %TRUE if the button should be drawn with a recessed shadow.
 * 
 * Sets whether the button is currently drawn as down or not. This is 
 * purely a visual setting, and is meant only for use by derived widgets
 * such as #BtkToggleButton.
 **/
void
_btk_button_set_depressed (BtkButton *button,
			   gboolean   depressed)
{
  BtkWidget *widget = BTK_WIDGET (button);

  depressed = depressed != FALSE;

  if (depressed != button->depressed)
    {
      button->depressed = depressed;
      btk_widget_queue_resize (widget);
    }
}

static void
btk_button_update_state (BtkButton *button)
{
  gboolean depressed, touchscreen;
  BtkStateType new_state;

  g_object_get (btk_widget_get_settings (BTK_WIDGET (button)),
                "btk-touchscreen-mode", &touchscreen,
                NULL);

  if (button->activate_timeout)
    depressed = button->depress_on_activate;
  else
    depressed = button->in_button && button->button_down;

  if (!touchscreen && button->in_button && (!button->button_down || !depressed))
    new_state = BTK_STATE_PRELIGHT;
  else
    new_state = depressed ? BTK_STATE_ACTIVE : BTK_STATE_NORMAL;

  _btk_button_set_depressed (button, depressed); 
  btk_widget_set_state (BTK_WIDGET (button), new_state);
}

static void 
show_image_change_notify (BtkButton *button)
{
  BtkButtonPrivate *priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->image) 
    {
      if (show_image (button))
	btk_widget_show (priv->image);
      else
	btk_widget_hide (priv->image);
    }
}

static void
traverse_container (BtkWidget *widget,
		    gpointer   data)
{
  if (BTK_IS_BUTTON (widget))
    show_image_change_notify (BTK_BUTTON (widget));
  else if (BTK_IS_CONTAINER (widget))
    btk_container_forall (BTK_CONTAINER (widget), traverse_container, NULL);
}

static void
btk_button_setting_changed (BtkSettings *settings)
{
  GList *list, *l;

  list = btk_window_list_toplevels ();

  for (l = list; l; l = l->next)
    btk_container_forall (BTK_CONTAINER (l->data), 
			  traverse_container, NULL);

  g_list_free (list);
}


static void
btk_button_screen_changed (BtkWidget *widget,
			   BdkScreen *previous_screen)
{
  BtkButton *button;
  BtkSettings *settings;
  guint show_image_connection;

  if (!btk_widget_has_screen (widget))
    return;

  button = BTK_BUTTON (widget);

  /* If the button is being pressed while the screen changes the
    release might never occur, so we reset the state. */
  if (button->button_down)
    {
      button->button_down = FALSE;
      btk_button_update_state (button);
    }

  settings = btk_widget_get_settings (widget);

  show_image_connection = 
    GPOINTER_TO_UINT (g_object_get_data (B_OBJECT (settings), 
					 "btk-button-connection"));
  
  if (show_image_connection)
    return;

  show_image_connection =
    g_signal_connect (settings, "notify::btk-button-images",
		      G_CALLBACK (btk_button_setting_changed), NULL);
  g_object_set_data (B_OBJECT (settings), 
		     I_("btk-button-connection"),
		     GUINT_TO_POINTER (show_image_connection));

  show_image_change_notify (button);
}

static void
btk_button_state_changed (BtkWidget    *widget,
                          BtkStateType  previous_state)
{
  BtkButton *button = BTK_BUTTON (widget);

  if (!btk_widget_is_sensitive (widget))
    {
      button->in_button = FALSE;
      btk_real_button_released (button);
    }
}

static void
btk_button_grab_notify (BtkWidget *widget,
			gboolean   was_grabbed)
{
  BtkButton *button = BTK_BUTTON (widget);
  gboolean save_in;

  if (!was_grabbed)
    {
      save_in = button->in_button;
      button->in_button = FALSE; 
      btk_real_button_released (button);
      if (save_in != button->in_button)
        {
          button->in_button = save_in;
          btk_button_update_state (button);
        }
    }
}

/**
 * btk_button_set_image:
 * @button: a #BtkButton
 * @image: a widget to set as the image for the button
 *
 * Set the image of @button to the given widget. Note that
 * it depends on the #BtkSettings:btk-button-images setting whether the
 * image will be displayed or not, you don't have to call
 * btk_widget_show() on @image yourself.
 *
 * Since: 2.6
 */ 
void
btk_button_set_image (BtkButton *button,
		      BtkWidget *image)
{
  BtkButtonPrivate *priv;

  g_return_if_fail (BTK_IS_BUTTON (button));
  g_return_if_fail (image == NULL || BTK_IS_WIDGET (image));

  priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->image && priv->image->parent)
    btk_container_remove (BTK_CONTAINER (priv->image->parent), priv->image);

  priv->image = image;
  priv->image_is_stock = (image == NULL);

  btk_button_construct_child (button);

  g_object_notify (B_OBJECT (button), "image");
}

/**
 * btk_button_get_image:
 * @button: a #BtkButton
 *
 * Gets the widget that is currenty set as the image of @button.
 * This may have been explicitly set by btk_button_set_image()
 * or constructed by btk_button_new_from_stock().
 *
 * Return value: (transfer none): a #BtkWidget or %NULL in case there is no image
 *
 * Since: 2.6
 */
BtkWidget *
btk_button_get_image (BtkButton *button)
{
  BtkButtonPrivate *priv;

  g_return_val_if_fail (BTK_IS_BUTTON (button), NULL);

  priv = BTK_BUTTON_GET_PRIVATE (button);
  
  return priv->image;
}

/**
 * btk_button_set_image_position:
 * @button: a #BtkButton
 * @position: the position
 *
 * Sets the position of the image relative to the text 
 * inside the button.
 *
 * Since: 2.10
 */ 
void
btk_button_set_image_position (BtkButton       *button,
			       BtkPositionType  position)
{

  BtkButtonPrivate *priv;

  g_return_if_fail (BTK_IS_BUTTON (button));
  g_return_if_fail (position >= BTK_POS_LEFT && position <= BTK_POS_BOTTOM);
  
  priv = BTK_BUTTON_GET_PRIVATE (button);

  if (priv->image_position != position)
    {
      priv->image_position = position;

      btk_button_construct_child (button);

      g_object_notify (B_OBJECT (button), "image-position");
    }
}

/**
 * btk_button_get_image_position:
 * @button: a #BtkButton
 *
 * Gets the position of the image relative to the text 
 * inside the button.
 *
 * Return value: the position
 *
 * Since: 2.10
 */
BtkPositionType
btk_button_get_image_position (BtkButton *button)
{
  BtkButtonPrivate *priv;

  g_return_val_if_fail (BTK_IS_BUTTON (button), BTK_POS_LEFT);

  priv = BTK_BUTTON_GET_PRIVATE (button);
  
  return priv->image_position;
}


/**
 * btk_button_get_event_window:
 * @button: a #BtkButton
 *
 * Returns the button's event window if it is realized, %NULL otherwise.
 * This function should be rarely needed.
 *
 * Return value: (transfer none): @button's event window.
 *
 * Since: 2.22
 */
BdkWindow*
btk_button_get_event_window (BtkButton *button)
{
  g_return_val_if_fail (BTK_IS_BUTTON (button), NULL);

  return button->event_window;
}

#define __BTK_BUTTON_C__
#include "btkaliasdef.c"  
