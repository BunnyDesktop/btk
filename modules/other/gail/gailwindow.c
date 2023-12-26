/* BAIL - The GNOME Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#include "config.h"

#include <string.h>

#undef BTK_DISABLE_DEPRECATED

#include <btk/btk.h>

#include "bailwindow.h"
#include "bailtoplevel.h"
#include "bail-private-macros.h"

enum {
  ACTIVATE,
  CREATE,
  DEACTIVATE,
  DESTROY,
  MAXIMIZE,
  MINIMIZE,
  MOVE,
  RESIZE,
  RESTORE,
  LAST_SIGNAL
};

static void bail_window_class_init (BailWindowClass *klass);

static void                  bail_window_init            (BailWindow   *accessible);

static void                  bail_window_real_initialize (BatkObject    *obj,
                                                          gpointer     data);
static void                  bail_window_finalize        (GObject      *object);

static const gchar*          bail_window_get_name        (BatkObject     *accessible);

static BatkObject*            bail_window_get_parent     (BatkObject     *accessible);
static gint                  bail_window_get_index_in_parent (BatkObject *accessible);
static gboolean              bail_window_real_focus_btk (BtkWidget     *widget,
                                                         BdkEventFocus *event);

static BatkStateSet*          bail_window_ref_state_set  (BatkObject     *accessible);
static BatkRelationSet*       bail_window_ref_relation_set  (BatkObject     *accessible);
static void                  bail_window_real_notify_btk (GObject      *obj,
                                                          GParamSpec   *pspec);
static gint                  bail_window_get_mdi_zorder (BatkComponent  *component);

static gboolean              bail_window_state_event_btk (BtkWidget           *widget,
                                                          BdkEventWindowState *event);

/* batkcomponent.h */
static void                  batk_component_interface_init (BatkComponentIface    *iface);

static void                  bail_window_get_extents      (BatkComponent         *component,
                                                           gint                 *x,
                                                           gint                 *y,
                                                           gint                 *width,
                                                           gint                 *height,
                                                           BatkCoordType         coord_type);
static void                  bail_window_get_size         (BatkComponent         *component,
                                                           gint                 *width,
                                                           gint                 *height);

static guint bail_window_signals [LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_CODE (BailWindow, bail_window, BAIL_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_COMPONENT, batk_component_interface_init))

static void
bail_window_class_init (BailWindowClass *klass)
{
  BailWidgetClass *widget_class;
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BatkObjectClass  *class = BATK_OBJECT_CLASS (klass);

  bobject_class->finalize = bail_window_finalize;

  widget_class = (BailWidgetClass*)klass;
  widget_class->focus_btk = bail_window_real_focus_btk;
  widget_class->notify_btk = bail_window_real_notify_btk;

  class->get_name = bail_window_get_name;
  class->get_parent = bail_window_get_parent;
  class->get_index_in_parent = bail_window_get_index_in_parent;
  class->ref_relation_set = bail_window_ref_relation_set;
  class->ref_state_set = bail_window_ref_state_set;
  class->initialize = bail_window_real_initialize;

  bail_window_signals [ACTIVATE] =
    g_signal_new ("activate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [CREATE] =
    g_signal_new ("create",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [DEACTIVATE] =
    g_signal_new ("deactivate",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [DESTROY] =
    g_signal_new ("destroy",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [MAXIMIZE] =
    g_signal_new ("maximize",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [MINIMIZE] =
    g_signal_new ("minimize",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [MOVE] =
    g_signal_new ("move",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [RESIZE] =
    g_signal_new ("resize",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  bail_window_signals [RESTORE] =
    g_signal_new ("restore",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, /* default signal handler */
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
bail_window_init (BailWindow   *accessible)
{
}

static void
bail_window_real_initialize (BatkObject *obj,
                             gpointer  data)
{
  BtkWidget *widget = BTK_WIDGET (data);
  BailWindow *window;

  /*
   * A BailWindow can be created for a BtkHandleBox or a BtkWindow
   */
  if (!BTK_IS_WINDOW (widget) &&
      !BTK_IS_HANDLE_BOX (widget))
    bail_return_if_fail (FALSE);

  BATK_OBJECT_CLASS (bail_window_parent_class)->initialize (obj, data);

  window = BAIL_WINDOW (obj);
  window->name_change_handler = 0;
  window->previous_name = g_strdup (btk_window_get_title (BTK_WINDOW (data)));

  g_signal_connect (data,
                    "window_state_event",
                    G_CALLBACK (bail_window_state_event_btk),
                    NULL);
  g_object_set_data (G_OBJECT (obj), "batk-component-layer",
                     GINT_TO_POINTER (BATK_LAYER_WINDOW));

  if (BTK_IS_FILE_SELECTION (widget))
    obj->role = BATK_ROLE_FILE_CHOOSER;
  else if (BTK_IS_COLOR_SELECTION_DIALOG (widget))
    obj->role = BATK_ROLE_COLOR_CHOOSER;
  else if (BTK_IS_FONT_SELECTION_DIALOG (widget))
    obj->role = BATK_ROLE_FONT_CHOOSER;
  else if (BTK_IS_MESSAGE_DIALOG (widget))
    obj->role = BATK_ROLE_ALERT;
  else if (BTK_IS_DIALOG (widget))
    obj->role = BATK_ROLE_DIALOG;
  else
    {
      const gchar *name;

      name = btk_widget_get_name (widget);
      if (name && (!strcmp (name, "btk-tooltip") ||
                   !strcmp (name, "btk-tooltips")))
        obj->role = BATK_ROLE_TOOL_TIP;
      else if (BTK_IS_PLUG (widget))
        obj->role = BATK_ROLE_PANEL;
      else if (BTK_WINDOW (widget)->type == BTK_WINDOW_POPUP)
        obj->role = BATK_ROLE_WINDOW;
      else
        obj->role = BATK_ROLE_FRAME;
    }

  /*
   * Notify that tooltip is showing
   */
  if (obj->role == BATK_ROLE_TOOL_TIP &&
      btk_widget_get_mapped (widget))
    batk_object_notify_state_change (obj, BATK_STATE_SHOWING, 1);
}

static void
bail_window_finalize (GObject *object)
{
  BailWindow* window = BAIL_WINDOW (object);

  if (window->name_change_handler)
    {
      g_source_remove (window->name_change_handler);
      window->name_change_handler = 0;
    }
  if (window->previous_name)
    {
      g_free (window->previous_name);
      window->previous_name = NULL;
    }

  G_OBJECT_CLASS (bail_window_parent_class)->finalize (object);
}

static const gchar*
bail_window_get_name (BatkObject *accessible)
{
  const gchar* name;

  name = BATK_OBJECT_CLASS (bail_window_parent_class)->get_name (accessible);
  if (name == NULL)
    {
      /*
       * Get the window title if it exists
       */
      BtkWidget* widget = BTK_ACCESSIBLE (accessible)->widget; 

      if (widget == NULL)
        /*
         * State is defunct
         */
        return NULL;

      bail_return_val_if_fail (BTK_IS_WIDGET (widget), NULL);

      if (BTK_IS_WINDOW (widget))
        {
          BtkWindow *window = BTK_WINDOW (widget);
 
          name = btk_window_get_title (window);
          if (name == NULL &&
              accessible->role == BATK_ROLE_TOOL_TIP)
            {
              BtkWidget *child;

              child = btk_bin_get_child (BTK_BIN (window));
              /* could be some kind of egg notification bubble thingy? */

              /* Handle new BTK+ GNOME 2.20 tooltips */
              if (BTK_IS_ALIGNMENT(child))
                {
                  child = btk_bin_get_child (BTK_BIN (child));
                  if (BTK_IS_BOX(child)) 
                    {
                      GList *children;
                      guint count;
                      children = btk_container_get_children (BTK_CONTAINER (child));
                      count = g_list_length (children);
                      if (count == 2) 
                        {
                          child = (BtkWidget *) g_list_nth_data (children, 1);
                        }
                      g_list_free (children);                
                    }
                }

              if (!BTK_IS_LABEL (child)) 
              { 
                  g_message ("BATK_ROLE_TOOLTIP object found, but doesn't look like a tooltip.");
                  return NULL;
              }
              name = btk_label_get_text (BTK_LABEL (child));
            }
        }
    }
  return name;
}

static BatkObject*
bail_window_get_parent (BatkObject *accessible)
{
  BatkObject* parent;

  parent = BATK_OBJECT_CLASS (bail_window_parent_class)->get_parent (accessible);

  return parent;
}

static gint
bail_window_get_index_in_parent (BatkObject *accessible)
{
  BtkWidget* widget = BTK_ACCESSIBLE (accessible)->widget; 
  BatkObject* batk_obj = batk_get_root ();
  gint index = -1;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return -1;

  bail_return_val_if_fail (BTK_IS_WIDGET (widget), -1);

  index = BATK_OBJECT_CLASS (bail_window_parent_class)->get_index_in_parent (accessible);
  if (index != -1)
    return index;

  if (BTK_IS_WINDOW (widget))
    {
      BtkWindow *window = BTK_WINDOW (widget);
      if (BAIL_IS_TOPLEVEL (batk_obj))
        {
	  BailToplevel* toplevel = BAIL_TOPLEVEL (batk_obj);
	  index = g_list_index (toplevel->window_list, window);
	}
      else
        {
	  int i, sibling_count = batk_object_get_n_accessible_children (batk_obj);
	  for (i = 0; i < sibling_count && index == -1; ++i)
	    {
	      BatkObject *child = batk_object_ref_accessible_child (batk_obj, i);
	      if (accessible == child) index = i;
	      g_object_unref (G_OBJECT (child));
	    }
	}
    }
  return index;
}

static gboolean
bail_window_real_focus_btk (BtkWidget     *widget,
                            BdkEventFocus *event)
{
  BatkObject* obj;

  obj = btk_widget_get_accessible (widget);
  batk_object_notify_state_change (obj, BATK_STATE_ACTIVE, event->in);

  return FALSE;
}

static BatkRelationSet*
bail_window_ref_relation_set (BatkObject *obj)
{
  BtkWidget *widget;
  BatkRelationSet *relation_set;
  BatkObject *array[1];
  BatkRelation* relation;
  BtkWidget *current_widget;

  bail_return_val_if_fail (BAIL_IS_WIDGET (obj), NULL);

  widget = BTK_ACCESSIBLE (obj)->widget;
  if (widget == NULL)
    /*
     * State is defunct
     */
    return NULL;

  relation_set = BATK_OBJECT_CLASS (bail_window_parent_class)->ref_relation_set (obj);

  if (batk_object_get_role (obj) == BATK_ROLE_TOOL_TIP)
    {
      relation = batk_relation_set_get_relation_by_type (relation_set, BATK_RELATION_POPUP_FOR);

      if (relation)
        {
          batk_relation_set_remove (relation_set, relation);
        }
      if (btk_widget_get_visible(widget) && btk_tooltips_get_info_from_tip_window (BTK_WINDOW (widget), NULL, &current_widget))
        {
          array [0] = btk_widget_get_accessible (current_widget);

          relation = batk_relation_new (array, 1, BATK_RELATION_POPUP_FOR);
          batk_relation_set_add (relation_set, relation);
          g_object_unref (relation);
        }
    }
  return relation_set;
}

static BatkStateSet*
bail_window_ref_state_set (BatkObject *accessible)
{
  BatkStateSet *state_set;
  BtkWidget *widget;
  BtkWindow *window;
  BdkWindowState state;

  state_set = BATK_OBJECT_CLASS (bail_window_parent_class)->ref_state_set (accessible);
  widget = BTK_ACCESSIBLE (accessible)->widget;
 
  if (widget == NULL)
    return state_set;

  window = BTK_WINDOW (widget);

  if (window->has_focus)
    batk_state_set_add_state (state_set, BATK_STATE_ACTIVE);

  if (widget->window)
    {
      state = bdk_window_get_state (widget->window);
      if (state & BDK_WINDOW_STATE_ICONIFIED)
        batk_state_set_add_state (state_set, BATK_STATE_ICONIFIED);
    } 
  if (btk_window_get_modal (window))
    batk_state_set_add_state (state_set, BATK_STATE_MODAL);

  if (btk_window_get_resizable (window))
    batk_state_set_add_state (state_set, BATK_STATE_RESIZABLE);
 
  return state_set;
}

static gboolean
idle_notify_name_change (gpointer data)
{
  BailWindow *window;
  BatkObject *obj;

  window = BAIL_WINDOW (data);
  window->name_change_handler = 0;
  if (BTK_ACCESSIBLE (window)->widget == NULL)
    return FALSE;

  obj = BATK_OBJECT (window);
  if (obj->name == NULL)
    {
    /*
     * The title has changed so notify a change in accessible-name
     */
      g_object_notify (G_OBJECT (obj), "accessible-name");
    }
  g_signal_emit_by_name (obj, "visible_data_changed");

  return FALSE;
}

static void
bail_window_real_notify_btk (GObject		*obj,
                             GParamSpec		*pspec)
{
  BtkWidget *widget = BTK_WIDGET (obj);
  BatkObject* batk_obj = btk_widget_get_accessible (widget);
  BailWindow *window = BAIL_WINDOW (batk_obj);
  const gchar *name;
  gboolean name_changed = FALSE;

  if (strcmp (pspec->name, "title") == 0)
    {
      name = btk_window_get_title (BTK_WINDOW (widget));
      if (name)
        {
         if (window->previous_name == NULL ||
             strcmp (name, window->previous_name) != 0)
           name_changed = TRUE;
        }
      else if (window->previous_name != NULL)
        name_changed = TRUE;

      if (name_changed)
        {
          g_free (window->previous_name);
          window->previous_name = g_strdup (name);
       
          if (window->name_change_handler == 0)
            window->name_change_handler = bdk_threads_add_idle (idle_notify_name_change, batk_obj);
        }
    }
  else
    BAIL_WIDGET_CLASS (bail_window_parent_class)->notify_btk (obj, pspec);
}

static gboolean
bail_window_state_event_btk (BtkWidget           *widget,
                             BdkEventWindowState *event)
{
  BatkObject* obj;

  obj = btk_widget_get_accessible (widget);
  batk_object_notify_state_change (obj, BATK_STATE_ICONIFIED,
                         (event->new_window_state & BDK_WINDOW_STATE_ICONIFIED) != 0);
  return FALSE;
}

static void
batk_component_interface_init (BatkComponentIface *iface)
{
  iface->get_extents = bail_window_get_extents;
  iface->get_size = bail_window_get_size;
  iface->get_mdi_zorder = bail_window_get_mdi_zorder;
}

static void
bail_window_get_extents (BatkComponent  *component,
                         gint          *x,
                         gint          *y,
                         gint          *width,
                         gint          *height,
                         BatkCoordType  coord_type)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget; 
  BdkRectangle rect;
  gint x_toplevel, y_toplevel;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return;

  bail_return_if_fail (BTK_IS_WINDOW (widget));

  if (!btk_widget_is_toplevel (widget))
    {
      BatkComponentIface *parent_iface;

      parent_iface = (BatkComponentIface *) g_type_interface_peek_parent (BATK_COMPONENT_GET_IFACE (component));
      parent_iface->get_extents (component, x, y, width, height, coord_type);
      return;
    }

  bdk_window_get_frame_extents (widget->window, &rect);

  *width = rect.width;
  *height = rect.height;
  if (!btk_widget_is_drawable (widget))
    {
      *x = G_MININT;
      *y = G_MININT;
      return;
    }
  *x = rect.x;
  *y = rect.y;
  if (coord_type == BATK_XY_WINDOW)
    {
      bdk_window_get_origin (widget->window, &x_toplevel, &y_toplevel);
      *x -= x_toplevel;
      *y -= y_toplevel;
    }
}

static void
bail_window_get_size (BatkComponent *component,
                      gint         *width,
                      gint         *height)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget; 
  BdkRectangle rect;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return;

  bail_return_if_fail (BTK_IS_WINDOW (widget));

  if (!btk_widget_is_toplevel (widget))
    {
      BatkComponentIface *parent_iface;

      parent_iface = (BatkComponentIface *) g_type_interface_peek_parent (BATK_COMPONENT_GET_IFACE (component));
      parent_iface->get_size (component, width, height);
      return;
    }
  bdk_window_get_frame_extents (widget->window, &rect);

  *width = rect.width;
  *height = rect.height;
}

#if defined (BDK_WINDOWING_X11)

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <bdk/x11/bdkx.h>

/* _NET_CLIENT_LIST_STACKING monitoring */

typedef struct {
  Window     *stacked_windows;
  int         stacked_windows_len;
  BdkWindow  *root_window;
  guint       update_handler;
  int        *desktop;
  guint       update_desktop_handler;
  gboolean   *desktop_changed;

  guint       screen_initialized : 1;
  guint       update_stacked_windows : 1;
} BailScreenInfo;

static BailScreenInfo *bail_screens = NULL;
static int             num_screens = 0;
static Atom            _net_client_list_stacking = None;
static Atom            _net_wm_desktop = None;

static gint
get_window_desktop (Window window)
{
  Atom            ret_type;
  int             format;
  gulong          nitems;
  gulong          bytes_after;
  guchar         *cardinals;
  int             error;
  int             result;
  int             desktop;

  if (_net_wm_desktop == None)
    _net_wm_desktop =
		XInternAtom (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()), "_NET_WM_DESKTOP", False);

  bdk_error_trap_push ();
  result = XGetWindowProperty (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()), window, _net_wm_desktop,
                               0, G_MAXLONG,
                               False, XA_CARDINAL,
                               &ret_type, &format, &nitems,
                               &bytes_after, &cardinals);
  error = bdk_error_trap_pop();
  /* nitems < 1 will occur if the property is not set */
  if (error != Success || result != Success || nitems < 1)
    return -1;

  desktop = *cardinals;

  XFree (cardinals);
  if (nitems != 1)
    return -1;
  return desktop;
}

static void
free_screen_info (BailScreenInfo *info)
{
  if (info->stacked_windows)
    XFree (info->stacked_windows);
  if (info->desktop)
    g_free (info->desktop);
  if (info->desktop_changed)
    g_free (info->desktop_changed);

  info->stacked_windows = NULL;
  info->stacked_windows_len = 0;
  info->desktop = NULL;
  info->desktop_changed = NULL;
}

static gboolean
get_stacked_windows (BailScreenInfo *info)
{
  Atom    ret_type;
  int     format;
  gulong  nitems;
  gulong  bytes_after;
  guchar *data;
  int     error;
  int     result;
  int     i;
  int     j;
  int    *desktops;
  gboolean *desktops_changed;

  if (_net_client_list_stacking == None)
    _net_client_list_stacking =
		XInternAtom (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()), "_NET_CLIENT_LIST_STACKING", False);

  bdk_error_trap_push ();
  ret_type = None;
  result = XGetWindowProperty (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()),
                               BDK_WINDOW_XWINDOW (info->root_window),
                               _net_client_list_stacking,
                               0, G_MAXLONG,
                               False, XA_WINDOW, &ret_type, &format, &nitems,
                               &bytes_after, &data);
  error = bdk_error_trap_pop ();
  /* nitems < 1 will occur if the property is not set */
  if (error != Success || result != Success || nitems < 1)
    {
      free_screen_info (info);
      return FALSE;
    }

  if (ret_type != XA_WINDOW)
    {
      XFree (data);
      free_screen_info (info);
      return FALSE;
    }

  desktops = g_malloc0 (nitems * sizeof (int));
  desktops_changed = g_malloc0 (nitems * sizeof (gboolean));
  for (i = 0; i < nitems; i++)
    {
      gboolean window_found = FALSE;

      for (j = 0; j < info->stacked_windows_len; j++)
        {
          if (info->stacked_windows [j] == data [i])
            {
              desktops [i] = info->desktop [j];
              desktops_changed [i] = info->desktop_changed [j];
              window_found = TRUE;
              break;
            }
        }
      if (!window_found)
        {
          desktops [i] = get_window_desktop (data [i]);
          desktops_changed [i] = FALSE;
        }
    }
  free_screen_info (info);
  info->stacked_windows = (Window*) data;
  info->stacked_windows_len = nitems;
  info->desktop = desktops;
  info->desktop_changed = desktops_changed;

  return TRUE;
}

static gboolean
update_screen_info (gpointer data)
{
  int screen_n = GPOINTER_TO_INT (data);

  bail_screens [screen_n].update_handler = 0;
  bail_screens [screen_n].update_stacked_windows = FALSE;

  get_stacked_windows (&bail_screens [screen_n]);

  return FALSE;
}

static gboolean
update_desktop_info (gpointer data)
{
  int screen_n = GPOINTER_TO_INT (data);
  BailScreenInfo *info;
  int i;

  info = &bail_screens [screen_n];
  info->update_desktop_handler = 0;

  for (i = 0; i < info->stacked_windows_len; i++)
    {
      if (info->desktop_changed [i])
        {
          info->desktop [i] = get_window_desktop (info->stacked_windows [i]);
          info->desktop_changed [i] = FALSE;
        }
    }

  return FALSE;
}

static BdkFilterReturn
filter_func (BdkXEvent *bdkxevent,
	     BdkEvent  *event,
	     gpointer   data)
{
  XEvent *xevent = bdkxevent;

  if (xevent->type == PropertyNotify)
    {
      if (xevent->xproperty.atom == _net_client_list_stacking)
        {
          int     screen_n;
          BdkWindow *window;

          window = event->any.window;

          if (window)
            {
              screen_n = bdk_screen_get_number (bdk_window_get_screen (window));

              bail_screens [screen_n].update_stacked_windows = TRUE;
              if (!bail_screens [screen_n].update_handler)
                {
                  bail_screens [screen_n].update_handler = bdk_threads_add_idle (update_screen_info,
	        						                 GINT_TO_POINTER (screen_n));
                }
            }
        }
      else if (xevent->xproperty.atom == _net_wm_desktop)
        {
          int     i;
          int     j;
          BailScreenInfo *info;

          for (i = 0; i < num_screens; i++)
            {
              info = &bail_screens [i];
              for (j = 0; j < info->stacked_windows_len; j++)
                {
                  if (xevent->xany.window == info->stacked_windows [j])
                    {
                      info->desktop_changed [j] = TRUE;
                      if (!info->update_desktop_handler)
                        {
                          info->update_desktop_handler = bdk_threads_add_idle (update_desktop_info,
                                                                               GINT_TO_POINTER (i));
                        }
                      break;
                    }
                }
            }
        }
    }
  return BDK_FILTER_CONTINUE;
}

static void
display_closed (BdkDisplay *display,
		gboolean    is_error)
{
  int i;

  for (i = 0; i < num_screens; i++)
    {
      if (bail_screens [i].update_handler)
	{
	  g_source_remove (bail_screens [i].update_handler);
	  bail_screens [i].update_handler = 0;
	}

      if (bail_screens [i].update_desktop_handler)
	{
	  g_source_remove (bail_screens [i].update_desktop_handler);
	  bail_screens [i].update_desktop_handler = 0;
	}

      free_screen_info (&bail_screens [i]);
    }

  g_free (bail_screens);
  bail_screens = NULL;
  num_screens = 0;
}

static void
init_bail_screens (void)
{
  BdkDisplay *display;

  display = bdk_display_get_default ();

  num_screens = bdk_display_get_n_screens (display);

  bail_screens = g_new0 (BailScreenInfo, num_screens);
  bdk_window_add_filter (NULL, filter_func, NULL);

  g_signal_connect (display, "closed", G_CALLBACK (display_closed), NULL);
}

static void
init_bail_screen (BdkScreen *screen,
                  int        screen_n)
{
  XWindowAttributes attrs;

  bail_screens [screen_n].root_window = bdk_screen_get_root_window (screen);

  get_stacked_windows (&bail_screens [screen_n]);

  XGetWindowAttributes (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()),
			BDK_WINDOW_XWINDOW (bail_screens [screen_n].root_window),
			&attrs); 

  XSelectInput (BDK_DISPLAY_XDISPLAY (bdk_display_get_default ()),
		BDK_WINDOW_XWINDOW (bail_screens [screen_n].root_window),
		attrs.your_event_mask | PropertyChangeMask);
           
  bail_screens [screen_n].screen_initialized = TRUE;
}

static BailScreenInfo *
get_screen_info (BdkScreen *screen)
{
  int screen_n;

  bail_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);

  screen_n = bdk_screen_get_number (screen);

  if (bail_screens && bail_screens [screen_n].screen_initialized)
    return &bail_screens [screen_n];

  if (!bail_screens)
    init_bail_screens ();

  g_assert (bail_screens != NULL);

  init_bail_screen (screen, screen_n);

  g_assert (bail_screens [screen_n].screen_initialized);

  return &bail_screens [screen_n];
}

static gint
get_window_zorder (BdkWindow *window)
{
  BailScreenInfo *info;
  Window          xid;
  int             i;
  int             zorder;
  int             w_desktop;

  bail_return_val_if_fail (BDK_IS_WINDOW (window), -1);

  info = get_screen_info (bdk_window_get_screen (window));

  bail_return_val_if_fail (info->stacked_windows != NULL, -1);

  xid = BDK_WINDOW_XID (window);

  w_desktop = -1;
  for (i = 0; i < info->stacked_windows_len; i++)
    {
      if (info->stacked_windows [i] == xid)
        {
          w_desktop = info->desktop[i];
          break;
        }
    }
  if (w_desktop < 0)
    return w_desktop;

  zorder = 0;
  for (i = 0; i < info->stacked_windows_len; i++)
    {
      if (info->stacked_windows [i] == xid)
        {
          return zorder;
        }
      else
        {
          if (info->desktop[i] == w_desktop)
            zorder++;
        }
     }

  return -1;
}

static gint
bail_window_get_mdi_zorder (BatkComponent *component)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return -1;

  bail_return_val_if_fail (BTK_IS_WINDOW (widget), -1);

  return get_window_zorder (widget->window);
}

#elif defined (BDK_WINDOWING_WIN32)

static gint
bail_window_get_mdi_zorder (BatkComponent *component)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return -1;

  bail_return_val_if_fail (BTK_IS_WINDOW (widget), -1);

  return 0;			/* Punt, FIXME */
}

#else

static gint
bail_window_get_mdi_zorder (BatkComponent *component)
{
  BtkWidget *widget = BTK_ACCESSIBLE (component)->widget;

  if (widget == NULL)
    /*
     * State is defunct
     */
    return -1;

  bail_return_val_if_fail (BTK_IS_WINDOW (widget), -1);

  return 0;			/* Punt, FIXME */
}

#endif
