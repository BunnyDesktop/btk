/*
 * btkinfobar.c
 * This file is part of BTK+
 *
 * Copyright (C) 2005 - Paolo Maggi
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a
 * list of people on the btk Team.
 * See the gedit ChangeLog files for a list of changes.
 *
 * Modified by the BTK+ team, 2008-2009.
 */


#include "config.h"

#include <stdlib.h>

#include "btkinfobar.h"
#include "btkaccessible.h"
#include "btkbuildable.h"
#include "btkbox.h"
#include "btkvbbox.h"
#include "btklabel.h"
#include "btkbutton.h"
#include "btkenums.h"
#include "btkbindings.h"
#include "btkdialog.h"
#include "btkintl.h"
#include "btkprivate.h"
#include "btkstock.h"
#include "bdkkeysyms.h"
#include "btkalias.h"


/**
 * SECTION:btkinfobar
 * @short_description: Report important messages to the user
 * @include: btk/btk.h
 * @see_also: #BtkStatusbar, #BtkMessageDialog
 *
 * #BtkInfoBar is a widget that can be used to show messages to
 * the user without showing a dialog. It is often temporarily shown
 * at the top or bottom of a document. In contrast to #BtkDialog, which
 * has a horizontal action area at the bottom, #BtkInfoBar has a
 * vertical action area at the side.
 *
 * The API of #BtkInfoBar is very similar to #BtkDialog, allowing you
 * to add buttons to the action area with btk_info_bar_add_button() or
 * btk_info_bar_new_with_buttons(). The sensitivity of action widgets
 * can be controlled with btk_info_bar_set_response_sensitive().
 * To add widgets to the main content area of a #BtkInfoBar, use
 * btk_info_bar_get_content_area() and add your widgets to the container.
 *
 * Similar to #BtkMessageDialog, the contents of a #BtkInfoBar can by
 * classified as error message, warning, informational message, etc,
 * by using btk_info_bar_set_message_type(). BTK+ uses the message type
 * to determine the background color of the message area.
 *
 * <example>
 * <title>Simple BtkInfoBar usage.</title>
 * <programlisting>
 * /&ast; set up info bar &ast;/
 * info_bar = btk_info_bar_new ();
 * btk_widget_set_no_show_all (info_bar, TRUE);
 * message_label = btk_label_new ("");
 * btk_widget_show (message_label);
 * content_area = btk_info_bar_get_content_area (BTK_INFO_BAR (info_bar));
 * btk_container_add (BTK_CONTAINER (content_area), message_label);
 * btk_info_bar_add_button (BTK_INFO_BAR (info_bar),
 *                          BTK_STOCK_OK, BTK_RESPONSE_OK);
 * g_signal_connect (info_bar, "response",
 *                   G_CALLBACK (btk_widget_hide), NULL);
 * btk_table_attach (BTK_TABLE (table),
 *                   info_bar,
 *                   0, 1, 2, 3,
 *                   BTK_EXPAND | BTK_FILL,  0,
 *                   0,                      0);
 *
 * /&ast; ... &ast;/
 *
 * /&ast; show an error message &ast;/
 * btk_label_set_text (BTK_LABEL (message_label), error_message);
 * btk_info_bar_set_message_type (BTK_INFO_BAR (info_bar),
 *                                BTK_MESSAGE_ERROR);
 * btk_widget_show (info_bar);
 * </programlisting>
 * </example>
 *
 * <refsect2 id="BtkInfoBar-BUILDER-UI">
 * <title>BtkInfoBar as BtkBuildable</title>
 * <para>
 * The BtkInfoBar implementation of the BtkBuildable interface exposes
 * the content area and action area as internal children with the names
 * "content_area" and "action_area".
 * </para>
 * <para>
 * BtkInfoBar supports a custom &lt;action-widgets&gt; element, which
 * can contain multiple &lt;action-widget&gt; elements. The "response"
 * attribute specifies a numeric response, and the content of the element
 * is the id of widget (which should be a child of the dialogs @action_area).
 * </para>
 * </refsect2>
 */

#define BTK_INFO_BAR_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), \
                                BTK_TYPE_INFO_BAR, \
                                BtkInfoBarPrivate))

enum
{
  PROP_0,
  PROP_MESSAGE_TYPE
};

struct _BtkInfoBarPrivate
{
  BtkWidget *content_area;
  BtkWidget *action_area;

  BtkMessageType message_type;
};

typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  gint response_id;
};

enum
{
  RESPONSE,
  CLOSE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];


static void     btk_info_bar_set_property (GObject        *object,
                                           guint           prop_id,
                                           const GValue   *value,
                                           GParamSpec     *pspec);
static void     btk_info_bar_get_property (GObject        *object,
                                           guint           prop_id,
                                           GValue         *value,
                                           GParamSpec     *pspec);
static void     btk_info_bar_style_set    (BtkWidget      *widget,
                                           BtkStyle       *prev_style);
static gboolean btk_info_bar_expose       (BtkWidget      *widget,
                                           BdkEventExpose *event);
static void     btk_info_bar_buildable_interface_init     (BtkBuildableIface *iface);
static GObject *btk_info_bar_buildable_get_internal_child (BtkBuildable  *buildable,
                                                           BtkBuilder    *builder,
                                                           const gchar   *childname);
static gboolean  btk_info_bar_buildable_custom_tag_start   (BtkBuildable  *buildable,
                                                            BtkBuilder    *builder,
                                                            GObject       *child,
                                                            const gchar   *tagname,
                                                            GMarkupParser *parser,
                                                            gpointer      *data);
static void      btk_info_bar_buildable_custom_finished    (BtkBuildable  *buildable,
                                                            BtkBuilder    *builder,
                                                            GObject       *child,
                                                            const gchar   *tagname,
                                                            gpointer       user_data);


G_DEFINE_TYPE_WITH_CODE (BtkInfoBar, btk_info_bar, BTK_TYPE_HBOX,
                         G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
                                                btk_info_bar_buildable_interface_init))

static void
btk_info_bar_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BtkInfoBar *info_bar;
  BtkInfoBarPrivate *priv;

  info_bar = BTK_INFO_BAR (object);
  priv = BTK_INFO_BAR_GET_PRIVATE (info_bar);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      btk_info_bar_set_message_type (info_bar, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_info_bar_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BtkInfoBar *info_bar;
  BtkInfoBarPrivate *priv;

  info_bar = BTK_INFO_BAR (object);
  priv = BTK_INFO_BAR_GET_PRIVATE (info_bar);

  switch (prop_id)
    {
    case PROP_MESSAGE_TYPE:
      g_value_set_enum (value, btk_info_bar_get_message_type (info_bar));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_info_bar_finalize (GObject *object)
{
  G_OBJECT_CLASS (btk_info_bar_parent_class)->finalize (object);
}

static void
response_data_free (gpointer data)
{
  g_slice_free (ResponseData, data);
}

static ResponseData *
get_response_data (BtkWidget *widget,
                   gboolean   create)
{
  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "btk-info-bar-response-data");

  if (ad == NULL && create)
    {
      ad = g_slice_new (ResponseData);

      g_object_set_data_full (G_OBJECT (widget),
                              I_("btk-info-bar-response-data"),
                              ad,
                              response_data_free);
    }

  return ad;
}

static BtkWidget *
find_button (BtkInfoBar *info_bar,
             gint        response_id)
{
  GList *children, *list;
  BtkWidget *child = NULL;

  children = btk_container_get_children (BTK_CONTAINER (info_bar->priv->action_area));

  for (list = children; list; list = list->next)
    {
      ResponseData *rd = get_response_data (list->data, FALSE);

      if (rd && rd->response_id == response_id)
        {
          child = list->data;
          break;
        }
    }

  g_list_free (children);

  return child;
}

static void
btk_info_bar_close (BtkInfoBar *info_bar)
{
  if (!find_button (info_bar, BTK_RESPONSE_CANCEL))
    return;

  btk_info_bar_response (BTK_INFO_BAR (info_bar),
                         BTK_RESPONSE_CANCEL);
}

static gboolean
btk_info_bar_expose (BtkWidget      *widget,
                     BdkEventExpose *event)
{
  BtkInfoBarPrivate *priv = BTK_INFO_BAR_GET_PRIVATE (widget);
  const char* type_detail[] = {
    "infobar-info",
    "infobar-warning",
    "infobar-question",
    "infobar-error",
    "infobar"
  };

  if (priv->message_type != BTK_MESSAGE_OTHER)
    {
      const char *detail;

      detail = type_detail[priv->message_type];

      btk_paint_box (widget->style,
                     widget->window,
                     BTK_STATE_NORMAL,
                     BTK_SHADOW_OUT,
                     NULL,
                     widget,
                     detail,
                     widget->allocation.x,
                     widget->allocation.y,
                     widget->allocation.width,
                     widget->allocation.height);
    }

  if (BTK_WIDGET_CLASS (btk_info_bar_parent_class)->expose_event)
    BTK_WIDGET_CLASS (btk_info_bar_parent_class)->expose_event (widget, event);

  return FALSE;
}

static void
btk_info_bar_class_init (BtkInfoBarClass *klass)
{
  BtkWidgetClass *widget_class;
  GObjectClass *object_class;
  BtkBindingSet *binding_set;

  widget_class = BTK_WIDGET_CLASS (klass);
  object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = btk_info_bar_get_property;
  object_class->set_property = btk_info_bar_set_property;
  object_class->finalize = btk_info_bar_finalize;

  widget_class->style_set = btk_info_bar_style_set;
  widget_class->expose_event = btk_info_bar_expose;

  klass->close = btk_info_bar_close;

  /**
   * BtkInfoBar:message-type:
   *
   * The type of the message.
   *
   * The type is used to determine the colors to use in the info bar.
   * The following symbolic color names can by used to customize
   * these colors:
   * "info_fg_color", "info_bg_color",
   * "warning_fg_color", "warning_bg_color",
   * "question_fg_color", "question_bg_color",
   * "error_fg_color", "error_bg_color".
   * "other_fg_color", "other_bg_color".
   *
   * If the type is #BTK_MESSAGE_OTHER, no info bar is painted but the
   * colors are still set.
   *
   * Since: 2.18
   */
  g_object_class_install_property (object_class,
                                   PROP_MESSAGE_TYPE,
                                   g_param_spec_enum ("message-type",
                                                      P_("Message Type"),
                                                      P_("The type of message"),
                                                      BTK_TYPE_MESSAGE_TYPE,
                                                      BTK_MESSAGE_INFO,
                                                      BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  /**
   * BtkInfoBar::response:
   * @info_bar: the object on which the signal is emitted
   * @response_id: the response ID
   *
   * Emitted when an action widget is clicked or the application programmer
   * calls btk_dialog_response(). The @response_id depends on which action
   * widget was clicked.
   *
   * Since: 2.18
   */
  signals[RESPONSE] = g_signal_new (I_("response"),
                                    G_OBJECT_CLASS_TYPE (klass),
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET (BtkInfoBarClass, response),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__INT,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);

  /**
   * BtkInfoBar::close:
   *
   * The ::close signal is a
   * <link linkend="keybinding-signals">keybinding signal</link>
   * which gets emitted when the user uses a keybinding to dismiss
   * the info bar.
   *
   * The default binding for this signal is the Escape key.
   *
   * Since: 2.18
   */
  signals[CLOSE] =  g_signal_new (I_("close"),
                                  G_OBJECT_CLASS_TYPE (klass),
                                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                  G_STRUCT_OFFSET (BtkInfoBarClass, close),
                                  NULL, NULL,
                                  g_cclosure_marshal_VOID__VOID,
                                  G_TYPE_NONE, 0);

  /**
   * BtkInfoBar:content-area-border:
   *
   * The width of the border around the content
   * content area of the info bar.
   *
   * Since: 2.18
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-border",
                                                             P_("Content area border"),
                                                             P_("Width of border around the content area"),
                                                             0,
                                                             G_MAXINT,
                                                             8,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkInfoBar:content-area-spacing:
   *
   * The default spacing used between elements of the
   * content area of the info bar.
   *
   * Since: 2.18
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-area-spacing",
                                                             P_("Content area spacing"),
                                                             P_("Spacing between elements of the area"),
                                                             0,
                                                             G_MAXINT,
                                                             16,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkInfoBar:button-spacing:
   *
   * Spacing between buttons in the action area of the info bar.
   *
   * Since: 2.18
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("button-spacing",
                                                             P_("Button spacing"),
                                                             P_("Spacing between buttons"),
                                                             0,
                                                             G_MAXINT,
                                                             6,
                                                             BTK_PARAM_READABLE));

  /**
   * BtkInfoBar:action-area-border:
   *
   * Width of the border around the action area of the info bar.
   *
   * Since: 2.18
   */
  btk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("action-area-border",
                                                             P_("Action area border"),
                                                             P_("Width of border around the action area"),
                                                             0,
                                                             G_MAXINT,
                                                             5,
                                                             BTK_PARAM_READABLE));

  binding_set = btk_binding_set_by_class (klass);

  btk_binding_entry_add_signal (binding_set, BDK_Escape, 0, "close", 0);

  g_type_class_add_private (object_class, sizeof (BtkInfoBarPrivate));
}

static void
btk_info_bar_update_colors (BtkInfoBar *info_bar)
{
  BtkWidget *widget = (BtkWidget*)info_bar;
  BtkInfoBarPrivate *priv;
  BdkColor info_default_border_color     = { 0, 0xb800, 0xad00, 0x9d00 };
  BdkColor info_default_fill_color       = { 0, 0xff00, 0xff00, 0xbf00 };
  BdkColor warning_default_border_color  = { 0, 0xb000, 0x7a00, 0x2b00 };
  BdkColor warning_default_fill_color    = { 0, 0xfc00, 0xaf00, 0x3e00 };
  BdkColor question_default_border_color = { 0, 0x6200, 0x7b00, 0xd960 };
  BdkColor question_default_fill_color   = { 0, 0x8c00, 0xb000, 0xd700 };
  BdkColor error_default_border_color    = { 0, 0xa800, 0x2700, 0x2700 };
  BdkColor error_default_fill_color      = { 0, 0xf000, 0x3800, 0x3800 };
  BdkColor other_default_border_color    = { 0, 0xb800, 0xad00, 0x9d00 };
  BdkColor other_default_fill_color      = { 0, 0xff00, 0xff00, 0xbf00 };
  BdkColor *fg, *bg;
  BdkColor sym_fg, sym_bg;
  BtkStyle *style;
  const char* fg_color_name[] = {
    "info_fg_color",
    "warning_fg_color",
    "question_fg_color",
    "error_fg_color",
    "other_fg_color"
  };
  const char* bg_color_name[] = {
    "info_bg_color",
    "warning_bg_color",
    "question_bg_color",
    "error_bg_color",
    "other_bg_color"
  };

  priv = BTK_INFO_BAR_GET_PRIVATE (info_bar);
  style = btk_widget_get_style (widget);

  if (btk_style_lookup_color (style, fg_color_name[priv->message_type], &sym_fg) &&
      btk_style_lookup_color (style, bg_color_name[priv->message_type], &sym_bg))
    {
      fg = &sym_fg;
      bg = &sym_bg;
    }
  else
    {
      switch (priv->message_type)
        {
        case BTK_MESSAGE_INFO:
          fg = &info_default_border_color;
          bg = &info_default_fill_color;
          break;

        case BTK_MESSAGE_WARNING:
          fg = &warning_default_border_color;
          bg = &warning_default_fill_color;
          break;

        case BTK_MESSAGE_QUESTION:
          fg = &question_default_border_color;
          bg = &question_default_fill_color;
          break;

        case BTK_MESSAGE_ERROR:
          fg = &error_default_border_color;
          bg = &error_default_fill_color;
          break;

        case BTK_MESSAGE_OTHER:
          fg = &other_default_border_color;
          bg = &other_default_fill_color;
          break;

        default:
          g_assert_not_reached();
          fg = NULL;
          bg = NULL;
        }
    }

  if (!bdk_color_equal (bg, &widget->style->bg[BTK_STATE_NORMAL]))
    btk_widget_modify_bg (widget, BTK_STATE_NORMAL, bg);
  if (!bdk_color_equal (fg, &widget->style->fg[BTK_STATE_NORMAL]))
    btk_widget_modify_fg (widget, BTK_STATE_NORMAL, fg);
}

static void
btk_info_bar_style_set (BtkWidget *widget,
                        BtkStyle  *prev_style)
{
  BtkInfoBar *info_bar = BTK_INFO_BAR (widget);
  gint button_spacing;
  gint action_area_border;
  gint content_area_spacing;
  gint content_area_border;

  btk_widget_style_get (widget,
                        "button-spacing", &button_spacing,
                        "action-area-border", &action_area_border,
                        "content-area-spacing", &content_area_spacing,
                        "content-area-border", &content_area_border,
                        NULL);

  btk_box_set_spacing (BTK_BOX (info_bar->priv->action_area), button_spacing);
  btk_container_set_border_width (BTK_CONTAINER (info_bar->priv->action_area),
                                  action_area_border);
  btk_box_set_spacing (BTK_BOX (info_bar->priv->content_area), content_area_spacing);
  btk_container_set_border_width (BTK_CONTAINER (info_bar->priv->content_area),
                                  content_area_border);

  btk_info_bar_update_colors (info_bar);
}

static void
btk_info_bar_init (BtkInfoBar *info_bar)
{
  BtkWidget *content_area;
  BtkWidget *action_area;

  btk_widget_push_composite_child ();

  info_bar->priv = BTK_INFO_BAR_GET_PRIVATE (info_bar);

  content_area = btk_hbox_new (FALSE, 0);
  btk_widget_show (content_area);
  btk_box_pack_start (BTK_BOX (info_bar), content_area, TRUE, TRUE, 0);

  action_area = btk_vbutton_box_new ();
  btk_widget_show (action_area);
  btk_button_box_set_layout (BTK_BUTTON_BOX (action_area), BTK_BUTTONBOX_END);
  btk_box_pack_start (BTK_BOX (info_bar), action_area, FALSE, TRUE, 0);

  btk_widget_set_app_paintable (BTK_WIDGET (info_bar), TRUE);
  btk_widget_set_redraw_on_allocate (BTK_WIDGET (info_bar), TRUE);

  info_bar->priv->content_area = content_area;
  info_bar->priv->action_area = action_area;

  btk_widget_pop_composite_child ();
}

static BtkBuildableIface *parent_buildable_iface;

static void
btk_info_bar_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = btk_info_bar_buildable_get_internal_child;
  iface->custom_tag_start = btk_info_bar_buildable_custom_tag_start;
  iface->custom_finished = btk_info_bar_buildable_custom_finished;
}

static GObject *
btk_info_bar_buildable_get_internal_child (BtkBuildable *buildable,
                                           BtkBuilder   *builder,
                                           const gchar  *childname)
{
  if (strcmp (childname, "content_area") == 0)
    return G_OBJECT (BTK_INFO_BAR (buildable)->priv->content_area);
  else if (strcmp (childname, "action_area") == 0)
    return G_OBJECT (BTK_INFO_BAR (buildable)->priv->action_area);

  return parent_buildable_iface->get_internal_child (buildable,
                                                     builder,
                                                     childname);
}

static gint
get_response_for_widget (BtkInfoBar *info_bar,
                         BtkWidget  *widget)
{
  ResponseData *rd;

  rd = get_response_data (widget, FALSE);
  if (!rd)
    return BTK_RESPONSE_NONE;
  else
    return rd->response_id;
}

static void
action_widget_activated (BtkWidget  *widget,
                         BtkInfoBar *info_bar)
{
  gint response_id;

  response_id = get_response_for_widget (info_bar, widget);
  btk_info_bar_response (info_bar, response_id);
}

/**
 * btk_info_bar_add_action_widget:
 * @info_bar: a #BtkInfoBar
 * @child: an activatable widget
 * @response_id: response ID for @child
 *
 * Add an activatable widget to the action area of a #BtkInfoBar,
 * connecting a signal handler that will emit the #BtkInfoBar::response
 * signal on the message area when the widget is activated. The widget
 * is appended to the end of the message areas action area.
 *
 * Since: 2.18
 */
void
btk_info_bar_add_action_widget (BtkInfoBar *info_bar,
                                BtkWidget  *child,
                                gint        response_id)
{
  ResponseData *ad;
  guint signal_id;

  g_return_if_fail (BTK_IS_INFO_BAR (info_bar));
  g_return_if_fail (BTK_IS_WIDGET (child));

  ad = get_response_data (child, TRUE);

  ad->response_id = response_id;

  if (BTK_IS_BUTTON (child))
    signal_id = g_signal_lookup ("clicked", BTK_TYPE_BUTTON);
  else
    signal_id = BTK_WIDGET_GET_CLASS (child)->activate_signal;

  if (signal_id)
    {
      GClosure *closure;

      closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                       G_OBJECT (info_bar));
      g_signal_connect_closure_by_id (child, signal_id, 0, closure, FALSE);
    }
  else
    g_warning ("Only 'activatable' widgets can be packed into the action area of a BtkInfoBar");

  btk_box_pack_end (BTK_BOX (info_bar->priv->action_area),
                    child, FALSE, FALSE, 0);
  if (response_id == BTK_RESPONSE_HELP)
    btk_button_box_set_child_secondary (BTK_BUTTON_BOX (info_bar->priv->action_area),
                                        child, TRUE);
}

/**
 * btk_info_bar_get_action_area:
 * @info_bar: a #BtkInfoBar
 *
 * Returns the action area of @info_bar.
 *
 * Returns: (transfer none): the action area
 *
 * Since: 2.18
 */
BtkWidget*
btk_info_bar_get_action_area (BtkInfoBar *info_bar)
{
  g_return_val_if_fail (BTK_IS_INFO_BAR (info_bar), NULL);

  return info_bar->priv->action_area;
}

/**
 * btk_info_bar_get_content_area:
 * @info_bar: a #BtkInfoBar
 *
 * Returns the content area of @info_bar.
 *
 * Returns: (transfer none): the content area
 *
 * Since: 2.18
 */
BtkWidget*
btk_info_bar_get_content_area (BtkInfoBar *info_bar)
{
  g_return_val_if_fail (BTK_IS_INFO_BAR (info_bar), NULL);

  return info_bar->priv->content_area;
}

/**
 * btk_info_bar_add_button:
 * @info_bar: a #BtkInfoBar
 * @button_text: text of button, or stock ID
 * @response_id: response ID for the button
 *
 * Adds a button with the given text (or a stock button, if button_text
 * is a stock ID) and sets things up so that clicking the button will emit
 * the "response" signal with the given response_id. The button is appended
 * to the end of the info bars's action area. The button widget is
 * returned, but usually you don't need it.
 *
 * Returns: (transfer none): the button widget that was added
 *
 * Since: 2.18
 */
BtkWidget*
btk_info_bar_add_button (BtkInfoBar  *info_bar,
                         const gchar *button_text,
                         gint         response_id)
{
  BtkWidget *button;

  g_return_val_if_fail (BTK_IS_INFO_BAR (info_bar), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  button = btk_button_new_from_stock (button_text);

  btk_widget_set_can_default (button, TRUE);

  btk_widget_show (button);

  btk_info_bar_add_action_widget (info_bar, button, response_id);

  return button;
}

static void
add_buttons_valist (BtkInfoBar  *info_bar,
                    const gchar *first_button_text,
                    va_list      args)
{
  const gchar* text;
  gint response_id;

  g_return_if_fail (BTK_IS_INFO_BAR (info_bar));

  if (first_button_text == NULL)
    return;

  text = first_button_text;
  response_id = va_arg (args, gint);

  while (text != NULL)
    {
      btk_info_bar_add_button (info_bar, text, response_id);

      text = va_arg (args, gchar*);
      if (text == NULL)
        break;

      response_id = va_arg (args, int);
    }
}

/**
 * btk_info_bar_add_buttons:
 * @info_bar: a #BtkInfoBar
 * @first_button_text: button text or stock ID
 * @...: response ID for first button, then more text-response_id pairs,
 *     ending with %NULL
 *
 * Adds more buttons, same as calling btk_info_bar_add_button()
 * repeatedly. The variable argument list should be %NULL-terminated
 * as with btk_info_bar_new_with_buttons(). Each button must have both
 * text and response ID.
 *
 * Since: 2.18
 */
void
btk_info_bar_add_buttons (BtkInfoBar  *info_bar,
                          const gchar *first_button_text,
                          ...)
{
  va_list args;

  va_start (args, first_button_text);
  add_buttons_valist (info_bar, first_button_text, args);
  va_end (args);
}

/**
 * btk_info_bar_new:
 *
 * Creates a new #BtkInfoBar object.
 *
 * Returns: a new #BtkInfoBar object
 *
 * Since: 2.18
 */
BtkWidget *
btk_info_bar_new (void)
{
   return g_object_new (BTK_TYPE_INFO_BAR, NULL);
}

/**
 * btk_info_bar_new_with_buttons:
 * @first_button_text: (allow-none): stock ID or text to go in first button, or %NULL
 * @...: response ID for first button, then additional buttons, ending
 *    with %NULL
 *
 * Creates a new #BtkInfoBar with buttons. Button text/response ID
 * pairs should be listed, with a %NULL pointer ending the list.
 * Button text can be either a stock ID such as %BTK_STOCK_OK, or
 * some arbitrary text. A response ID can be any positive number,
 * or one of the values in the #BtkResponseType enumeration. If the
 * user clicks one of these dialog buttons, BtkInfoBar will emit
 * the "response" signal with the corresponding response ID.
 *
 * Returns: a new #BtkInfoBar
 */
BtkWidget*
btk_info_bar_new_with_buttons (const gchar *first_button_text,
                               ...)
{
  BtkInfoBar *info_bar;
  va_list args;

  info_bar = BTK_INFO_BAR (btk_info_bar_new ());

  va_start (args, first_button_text);
  add_buttons_valist (info_bar, first_button_text, args);
  va_end (args);

  return BTK_WIDGET (info_bar);
}

/**
 * btk_info_bar_set_response_sensitive:
 * @info_bar: a #BtkInfoBar
 * @response_id: a response ID
 * @setting: TRUE for sensitive
 *
 * Calls btk_widget_set_sensitive (widget, setting) for each
 * widget in the info bars's action area with the given response_id.
 * A convenient way to sensitize/desensitize dialog buttons.
 *
 * Since: 2.18
 */
void
btk_info_bar_set_response_sensitive (BtkInfoBar *info_bar,
                                     gint        response_id,
                                     gboolean    setting)
{
  GList *children, *list;

  g_return_if_fail (BTK_IS_INFO_BAR (info_bar));

  children = btk_container_get_children (BTK_CONTAINER (info_bar->priv->action_area));

  for (list = children; list; list = list->next)
    {
      BtkWidget *widget = list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        btk_widget_set_sensitive (widget, setting);
    }

  g_list_free (children);
}

/**
 * btk_info_bar_set_default_response:
 * @info_bar: a #BtkInfoBar
 * @response_id: a response ID
 *
 * Sets the last widget in the info bar's action area with
 * the given response_id as the default widget for the dialog.
 * Pressing "Enter" normally activates the default widget.
 *
 * Note that this function currently requires @info_bar to
 * be added to a widget hierarchy. 
 *
 * Since: 2.18
 */
void
btk_info_bar_set_default_response (BtkInfoBar *info_bar,
                                   gint        response_id)
{
  GList *children, *list;

  g_return_if_fail (BTK_IS_INFO_BAR (info_bar));

  children = btk_container_get_children (BTK_CONTAINER (info_bar->priv->action_area));

  for (list = children; list; list = list->next)
    {
      BtkWidget *widget = list->data;
      ResponseData *rd = get_response_data (widget, FALSE);

      if (rd && rd->response_id == response_id)
        btk_widget_grab_default (widget);
    }

  g_list_free (children);
}

/**
 * btk_info_bar_response:
 * @info_bar: a #BtkInfoBar
 * @response_id: a response ID
 *
 * Emits the 'response' signal with the given @response_id.
 *
 * Since: 2.18
 */
void
btk_info_bar_response (BtkInfoBar *info_bar,
                       gint        response_id)
{
  g_return_if_fail (BTK_IS_INFO_BAR (info_bar));

  g_signal_emit (info_bar, signals[RESPONSE], 0, response_id);
}

typedef struct
{
  gchar *widget_name;
  gchar *response_id;
} ActionWidgetInfo;

typedef struct
{
  BtkInfoBar *info_bar;
  BtkBuilder *builder;
  GSList *items;
  gchar *response;
} ActionWidgetsSubParserData;

static void
attributes_start_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          const gchar         **names,
                          const gchar         **values,
                          gpointer              user_data,
                          GError              **error)
{
  ActionWidgetsSubParserData *parser_data = (ActionWidgetsSubParserData*)user_data;
  guint i;

  if (strcmp (element_name, "action-widget") == 0)
    {
      for (i = 0; names[i]; i++)
        if (strcmp (names[i], "response") == 0)
          parser_data->response = g_strdup (values[i]);
    }
  else if (strcmp (element_name, "action-widgets") == 0)
    return;
  else
    g_warning ("Unsupported tag for BtkInfoBar: %s\n", element_name);
}

static void
attributes_text_element (GMarkupParseContext  *context,
                         const gchar          *text,
                         gsize                 text_len,
                         gpointer              user_data,
                         GError              **error)
{
  ActionWidgetsSubParserData *parser_data = (ActionWidgetsSubParserData*)user_data;
  ActionWidgetInfo *item;

  if (!parser_data->response)
    return;

  item = g_new (ActionWidgetInfo, 1);
  item->widget_name = g_strndup (text, text_len);
  item->response_id = parser_data->response;
  parser_data->items = g_slist_prepend (parser_data->items, item);
  parser_data->response = NULL;
}

static const GMarkupParser attributes_parser =
{
  attributes_start_element,
  NULL,
  attributes_text_element,
};

gboolean
btk_info_bar_buildable_custom_tag_start (BtkBuildable  *buildable,
                                         BtkBuilder    *builder,
                                         GObject       *child,
                                         const gchar   *tagname,
                                         GMarkupParser *parser,
                                         gpointer      *data)
{
  ActionWidgetsSubParserData *parser_data;

  if (child)
    return FALSE;

  if (strcmp (tagname, "action-widgets") == 0)
    {
      parser_data = g_slice_new0 (ActionWidgetsSubParserData);
      parser_data->info_bar = BTK_INFO_BAR (buildable);
      parser_data->items = NULL;

      *parser = attributes_parser;
      *data = parser_data;
      return TRUE;
    }

  return parent_buildable_iface->custom_tag_start (buildable, builder, child,
                                                   tagname, parser, data);
}

static void
btk_info_bar_buildable_custom_finished (BtkBuildable *buildable,
                                        BtkBuilder   *builder,
                                        GObject      *child,
                                        const gchar  *tagname,
                                        gpointer      user_data)
{
  GSList *l;
  ActionWidgetsSubParserData *parser_data;
  GObject *object;
  BtkInfoBar *info_bar;
  ResponseData *ad;
  guint signal_id;

  if (strcmp (tagname, "action-widgets"))
    {
      parent_buildable_iface->custom_finished (buildable, builder, child,
                                               tagname, user_data);
      return;
    }

  info_bar = BTK_INFO_BAR (buildable);
  parser_data = (ActionWidgetsSubParserData*)user_data;
  parser_data->items = g_slist_reverse (parser_data->items);

  for (l = parser_data->items; l; l = l->next)
    {
      ActionWidgetInfo *item = l->data;

      object = btk_builder_get_object (builder, item->widget_name);
      if (!object)
        {
          g_warning ("Unknown object %s specified in action-widgets of %s",
                     item->widget_name,
                     btk_buildable_get_name (BTK_BUILDABLE (buildable)));
          continue;
        }

      ad = get_response_data (BTK_WIDGET (object), TRUE);
      ad->response_id = atoi (item->response_id);

      if (BTK_IS_BUTTON (object))
        signal_id = g_signal_lookup ("clicked", BTK_TYPE_BUTTON);
      else
        signal_id = BTK_WIDGET_GET_CLASS (object)->activate_signal;

      if (signal_id)
        {
          GClosure *closure;

          closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                           G_OBJECT (info_bar));
          g_signal_connect_closure_by_id (object,
                                          signal_id,
                                          0,
                                          closure,
                                          FALSE);
        }

      if (ad->response_id == BTK_RESPONSE_HELP)
        btk_button_box_set_child_secondary (BTK_BUTTON_BOX (info_bar->priv->action_area),
                                            BTK_WIDGET (object), TRUE);

      g_free (item->widget_name);
      g_free (item->response_id);
      g_free (item);
    }
  g_slist_free (parser_data->items);
  g_slice_free (ActionWidgetsSubParserData, parser_data);
}

/**
 * btk_info_bar_set_message_type:
 * @info_bar: a #BtkInfoBar
 * @message_type: a #BtkMessageType
 *
 * Sets the message type of the message area.
 * BTK+ uses this type to determine what color to use
 * when drawing the message area.
 *
 * Since: 2.18
 */
void
btk_info_bar_set_message_type (BtkInfoBar     *info_bar,
                               BtkMessageType  message_type)
{
  BtkInfoBarPrivate *priv;
  BatkObject *batk_obj;

  g_return_if_fail (BTK_IS_INFO_BAR (info_bar));

  priv = BTK_INFO_BAR_GET_PRIVATE (info_bar);

  if (priv->message_type != message_type)
    {
      priv->message_type = message_type;

      btk_info_bar_update_colors (info_bar);
      btk_widget_queue_draw (BTK_WIDGET (info_bar));

      batk_obj = btk_widget_get_accessible (BTK_WIDGET (info_bar));
      if (BTK_IS_ACCESSIBLE (batk_obj))
        {
          BtkStockItem item;
          const char *stock_id = NULL;

          batk_object_set_role (batk_obj, BATK_ROLE_ALERT);

          switch (message_type)
            {
            case BTK_MESSAGE_INFO:
              stock_id = BTK_STOCK_DIALOG_INFO;
              break;

            case BTK_MESSAGE_QUESTION:
              stock_id = BTK_STOCK_DIALOG_QUESTION;
              break;

            case BTK_MESSAGE_WARNING:
              stock_id = BTK_STOCK_DIALOG_WARNING;
              break;

            case BTK_MESSAGE_ERROR:
              stock_id = BTK_STOCK_DIALOG_ERROR;
              break;

            case BTK_MESSAGE_OTHER:
              break;

            default:
              g_warning ("Unknown BtkMessageType %u", message_type);
              break;
            }

          if (stock_id)
            {
              btk_stock_lookup (stock_id, &item);
              batk_object_set_name (batk_obj, item.label);
            }
        }

      g_object_notify (G_OBJECT (info_bar), "message-type");
    }
}

/**
 * btk_info_bar_get_message_type:
 * @info_bar: a #BtkInfoBar
 *
 * Returns the message type of the message area.
 *
 * Returns: the message type of the message area.
 *
 * Since: 2.18
 */
BtkMessageType
btk_info_bar_get_message_type (BtkInfoBar *info_bar)
{
  BtkInfoBarPrivate *priv;

  g_return_val_if_fail (BTK_IS_INFO_BAR (info_bar), BTK_MESSAGE_OTHER);

  priv = BTK_INFO_BAR_GET_PRIVATE (info_bar);

  return priv->message_type;
}


#define __BTK_INFO_BAR_C__
#include "btkaliasdef.c"
