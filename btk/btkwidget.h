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

#ifndef __BTK_WIDGET_H__
#define __BTK_WIDGET_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkaccelgroup.h>
#include <btk/btkobject.h>
#include <btk/btkadjustment.h>
#include <btk/btkstyle.h>
#include <btk/btksettings.h>
#include <batk/batk.h>

B_BEGIN_DECLS

/**
 * BtkWidgetFlags:
 * @BTK_TOPLEVEL: widgets without a real parent, as there are #BtkWindow<!-- -->s and
 *  #BtkMenu<!-- -->s have this flag set throughout their lifetime.
 *  Toplevel widgets always contain their own #BdkWindow.
 * @BTK_NO_WINDOW: Indicative for a widget that does not provide its own #BdkWindow.
 *  Visible action (e.g. drawing) is performed on the parent's #BdkWindow.
 * @BTK_REALIZED: Set by btk_widget_realize(), unset by btk_widget_unrealize().
 *  A realized widget has an associated #BdkWindow.
 * @BTK_MAPPED: Set by btk_widget_map(), unset by btk_widget_unmap().
 *  Only realized widgets can be mapped. It means that bdk_window_show()
 *  has been called on the widgets window(s).
 * @BTK_VISIBLE: Set by btk_widget_show(), unset by btk_widget_hide(). Implies that a
 *  widget will be mapped as soon as its parent is mapped.
 * @BTK_SENSITIVE: Set and unset by btk_widget_set_sensitive().
 *  The sensitivity of a widget determines whether it will receive
 *  certain events (e.g. button or key presses). One premise for
 *  the widget's sensitivity is to have this flag set.
 * @BTK_PARENT_SENSITIVE: Set and unset by btk_widget_set_sensitive() operations on the
 *  parents of the widget.
 *  This is the second premise for the widget's sensitivity. Once
 *  it has %BTK_SENSITIVE and %BTK_PARENT_SENSITIVE set, its state is
 *  effectively sensitive. This is expressed (and can be examined) by
 *  the #BTK_WIDGET_IS_SENSITIVE macro.
 * @BTK_CAN_FOCUS: Determines whether a widget is able to handle focus grabs.
 * @BTK_HAS_FOCUS: Set by btk_widget_grab_focus() for widgets that also
 *  have %BTK_CAN_FOCUS set. The flag will be unset once another widget
 *  grabs the focus.
 * @BTK_CAN_DEFAULT: The widget is allowed to receive the default action via
 *  btk_widget_grab_default() and will reserve space to draw the default if possible
 * @BTK_HAS_DEFAULT: The widget currently is receiving the default action and
 *  should be drawn appropriately if possible
 * @BTK_HAS_GRAB: Set by btk_grab_add(), unset by btk_grab_remove(). It means that the
 *  widget is in the grab_widgets stack, and will be the preferred one for
 *  receiving events other than ones of cosmetic value.
 * @BTK_RC_STYLE: Indicates that the widget's style has been looked up through the rc
 *  mechanism. It does not imply that the widget actually had a style
 *  defined through the rc mechanism.
 * @BTK_COMPOSITE_CHILD: Indicates that the widget is a composite child of its parent; see
 *  btk_widget_push_composite_child(), btk_widget_pop_composite_child().
 * @BTK_NO_REPARENT: Unused since before BTK+ 1.2, will be removed in a future version.
 * @BTK_APP_PAINTABLE: Set and unset by btk_widget_set_app_paintable().
 *  Must be set on widgets whose window the application directly draws on,
 *  in order to keep BTK+ from overwriting the drawn stuff.  See
 *  <xref linkend="app-paintable-widgets"/> for a detailed
 *  description of this flag.
 * @BTK_RECEIVES_DEFAULT: The widget when focused will receive the default action and have
 *  %BTK_HAS_DEFAULT set even if there is a different widget set as default.
 * @BTK_DOUBLE_BUFFERED: Set and unset by btk_widget_set_double_buffered().
 *  Indicates that exposes done on the widget should be
 *  double-buffered.  See <xref linkend="double-buffering"/> for a
 *  detailed discussion of how double-buffering works in BTK+ and
 *  why you may want to disable it for special cases.
 * @BTK_NO_SHOW_ALL:
 *
 * Tells about certain properties of the widget.
 */
typedef enum
{
  BTK_TOPLEVEL         = 1 << 4,
  BTK_NO_WINDOW        = 1 << 5,
  BTK_REALIZED         = 1 << 6,
  BTK_MAPPED           = 1 << 7,
  BTK_VISIBLE          = 1 << 8,
  BTK_SENSITIVE        = 1 << 9,
  BTK_PARENT_SENSITIVE = 1 << 10,
  BTK_CAN_FOCUS        = 1 << 11,
  BTK_HAS_FOCUS        = 1 << 12,
  BTK_CAN_DEFAULT      = 1 << 13,
  BTK_HAS_DEFAULT      = 1 << 14,
  BTK_HAS_GRAB	       = 1 << 15,
  BTK_RC_STYLE	       = 1 << 16,
  BTK_COMPOSITE_CHILD  = 1 << 17,
#ifndef BTK_DISABLE_DEPRECATED
  BTK_NO_REPARENT      = 1 << 18,
#endif
  BTK_APP_PAINTABLE    = 1 << 19,
  BTK_RECEIVES_DEFAULT = 1 << 20,
  BTK_DOUBLE_BUFFERED  = 1 << 21,
  BTK_NO_SHOW_ALL      = 1 << 22
} BtkWidgetFlags;

/* Kinds of widget-specific help */
typedef enum
{
  BTK_WIDGET_HELP_TOOLTIP,
  BTK_WIDGET_HELP_WHATS_THIS
} BtkWidgetHelpType;

/* Macro for casting a pointer to a BtkWidget or BtkWidgetClass pointer.
 * Macros for testing whether `widget' or `klass' are of type BTK_TYPE_WIDGET.
 */
#define BTK_TYPE_WIDGET			  (btk_widget_get_type ())
#define BTK_WIDGET(widget)		  (G_TYPE_CHECK_INSTANCE_CAST ((widget), BTK_TYPE_WIDGET, BtkWidget))
#define BTK_WIDGET_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_WIDGET, BtkWidgetClass))
#define BTK_IS_WIDGET(widget)		  (G_TYPE_CHECK_INSTANCE_TYPE ((widget), BTK_TYPE_WIDGET))
#define BTK_IS_WIDGET_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_WIDGET))
#define BTK_WIDGET_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_WIDGET, BtkWidgetClass))

/* Macros for extracting various fields from BtkWidget and BtkWidgetClass.
 */
#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_TYPE:
 * @wid: a #BtkWidget.
 *
 * Gets the type of a widget.
 *
 * Deprecated: 2.20: Use G_OBJECT_TYPE() instead.
 */
#define BTK_WIDGET_TYPE(wid)		  (BTK_OBJECT_TYPE (wid))
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_STATE:
 * @wid: a #BtkWidget.
 *
 * Returns the current state of the widget, as a #BtkStateType.
 *
 * Deprecated: 2.20: Use btk_widget_get_state() instead.
 */
#define BTK_WIDGET_STATE(wid)		  (BTK_WIDGET (wid)->state)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_SAVED_STATE:
 * @wid: a #BtkWidget.
 *
 * Returns the saved state of the widget, as a #BtkStateType.
 *
 * The saved state will be restored when a widget gets sensitive
 * again, after it has been made insensitive with btk_widget_set_state()
 * or btk_widget_set_sensitive().
 *
 * Deprecated: 2.20: Do not used it.
 */
#define BTK_WIDGET_SAVED_STATE(wid)	  (BTK_WIDGET (wid)->saved_state)
#endif


/* Macros for extracting the widget flags from BtkWidget.
 */
/**
 * BTK_WIDGET_FLAGS:
 * @wid: a #BtkWidget.
 *
 * Returns the widget flags from @wid.
 *
 * Deprecated: 2.20: Use the proper function to test individual states:
 * btk_widget_get_app_paintable(), btk_widget_get_can_default(),
 * btk_widget_get_can_focus(), btk_widget_get_double_buffered(),
 * btk_widget_has_default(), btk_widget_is_drawable(),
 * btk_widget_has_focus(), btk_widget_has_grab(), btk_widget_get_mapped(),
 * btk_widget_get_has_window(), btk_widget_has_rc_style(),
 * btk_widget_get_realized(), btk_widget_get_receives_default(),
 * btk_widget_get_sensitive(), btk_widget_is_sensitive(),
 * btk_widget_is_toplevel() or btk_widget_get_visible().
 */
#define BTK_WIDGET_FLAGS(wid)		  (BTK_OBJECT_FLAGS (wid))
/* FIXME: Deprecating BTK_WIDGET_FLAGS requires fixing BTK internals. */

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_TOPLEVEL:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is a toplevel widget.
 *
 * Deprecated: 2.20: Use btk_widget_is_toplevel() instead.
 */
#define BTK_WIDGET_TOPLEVEL(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_TOPLEVEL) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_NO_WINDOW:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget doesn't have an own #BdkWindow.
 *
 * Deprecated: 2.20: Use btk_widget_get_has_window() instead.
 */
#define BTK_WIDGET_NO_WINDOW(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_NO_WINDOW) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_REALIZED:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is realized.
 *
 * Deprecated: 2.20: Use btk_widget_get_realized() instead.
 */
#define BTK_WIDGET_REALIZED(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_REALIZED) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_MAPPED:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is mapped.
 *
 * Deprecated: 2.20: Use btk_widget_get_mapped() instead.
 */
#define BTK_WIDGET_MAPPED(wid)		  ((BTK_WIDGET_FLAGS (wid) & BTK_MAPPED) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_VISIBLE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is visible.
 *
 * Deprecated: 2.20: Use btk_widget_get_visible() instead.
 */
#define BTK_WIDGET_VISIBLE(wid)		  ((BTK_WIDGET_FLAGS (wid) & BTK_VISIBLE) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_DRAWABLE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is mapped and visible.
 *
 * Deprecated: 2.20: Use btk_widget_is_drawable() instead.
 */
#define BTK_WIDGET_DRAWABLE(wid)	  (BTK_WIDGET_VISIBLE (wid) && BTK_WIDGET_MAPPED (wid))
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_SENSITIVE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the #BTK_SENSITIVE flag has be set on the widget.
 *
 * Deprecated: 2.20: Use btk_widget_get_sensitive() instead.
 */
#define BTK_WIDGET_SENSITIVE(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_SENSITIVE) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_PARENT_SENSITIVE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the #BTK_PARENT_SENSITIVE flag has be set on the widget.
 *
 * Deprecated: 2.20: Use btk_widget_get_sensitive() on the parent widget instead.
 */
#define BTK_WIDGET_PARENT_SENSITIVE(wid)  ((BTK_WIDGET_FLAGS (wid) & BTK_PARENT_SENSITIVE) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_IS_SENSITIVE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is effectively sensitive.
 *
 * Deprecated: 2.20: Use btk_widget_is_sensitive() instead.
 */
#define BTK_WIDGET_IS_SENSITIVE(wid)	  (BTK_WIDGET_SENSITIVE (wid) && \
					   BTK_WIDGET_PARENT_SENSITIVE (wid))
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_CAN_FOCUS:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is able to handle focus grabs.
 *
 * Deprecated: 2.20: Use btk_widget_get_can_focus() instead.
 */
#define BTK_WIDGET_CAN_FOCUS(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_CAN_FOCUS) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_HAS_FOCUS:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget has grabbed the focus and no other
 * widget has done so more recently.
 *
 * Deprecated: 2.20: Use btk_widget_has_focus() instead.
 */
#define BTK_WIDGET_HAS_FOCUS(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_HAS_FOCUS) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_CAN_DEFAULT:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is allowed to receive the default action
 * via btk_widget_grab_default().
 *
 * Deprecated: 2.20: Use btk_widget_get_can_default() instead.
 */
#define BTK_WIDGET_CAN_DEFAULT(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_CAN_DEFAULT) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_HAS_DEFAULT:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget currently is receiving the default action.
 *
 * Deprecated: 2.20: Use btk_widget_has_default() instead.
 */
#define BTK_WIDGET_HAS_DEFAULT(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_HAS_DEFAULT) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_HAS_GRAB:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is in the grab_widgets stack, and will be
 * the preferred one for receiving events other than ones of cosmetic value.
 *
 * Deprecated: 2.20: Use btk_widget_has_grab() instead.
 */
#define BTK_WIDGET_HAS_GRAB(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_HAS_GRAB) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_RC_STYLE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget's style has been looked up through the rc
 * mechanism.
 *
 * Deprecated: 2.20: Use btk_widget_has_rc_style() instead.
 */
#define BTK_WIDGET_RC_STYLE(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_RC_STYLE) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_COMPOSITE_CHILD:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget is a composite child of its parent.
 *
 * Deprecated: 2.20: Use the #BtkWidget:composite-child property instead.
 */
#define BTK_WIDGET_COMPOSITE_CHILD(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_COMPOSITE_CHILD) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_APP_PAINTABLE:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the #BTK_APP_PAINTABLE flag has been set on the widget.
 *
 * Deprecated: 2.20: Use btk_widget_get_app_paintable() instead.
 */
#define BTK_WIDGET_APP_PAINTABLE(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_APP_PAINTABLE) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_RECEIVES_DEFAULT:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the widget when focused will receive the default action
 * even if there is a different widget set as default.
 *
 * Deprecated: 2.20: Use btk_widget_get_receives_default() instead.
 */
#define BTK_WIDGET_RECEIVES_DEFAULT(wid)  ((BTK_WIDGET_FLAGS (wid) & BTK_RECEIVES_DEFAULT) != 0)
#endif

#ifndef BTK_DISABLE_DEPRECATED
/**
 * BTK_WIDGET_DOUBLE_BUFFERED:
 * @wid: a #BtkWidget.
 *
 * Evaluates to %TRUE if the #BTK_DOUBLE_BUFFERED flag has been set on the widget.
 *
 * Deprecated: 2.20: Use btk_widget_get_double_buffered() instead.
 */
#define BTK_WIDGET_DOUBLE_BUFFERED(wid)	  ((BTK_WIDGET_FLAGS (wid) & BTK_DOUBLE_BUFFERED) != 0)
#endif


/* Macros for setting and clearing widget flags.
 */
/**
 * BTK_WIDGET_SET_FLAGS:
 * @wid: a #BtkWidget.
 * @flag: the flags to set.
 *
 * Turns on certain widget flags.
 *
 * Deprecated: 2.22: Use the proper function instead: btk_widget_set_app_paintable(),
 *   btk_widget_set_can_default(), btk_widget_set_can_focus(),
 *   btk_widget_set_double_buffered(), btk_widget_set_has_window(),
 *   btk_widget_set_mapped(), btk_widget_set_no_show_all(),
 *   btk_widget_set_realized(), btk_widget_set_receives_default(),
 *   btk_widget_set_sensitive() or btk_widget_set_visible().
 *
 */
#define BTK_WIDGET_SET_FLAGS(wid,flag)	  B_STMT_START{ (BTK_WIDGET_FLAGS (wid) |= (flag)); }B_STMT_END
/* FIXME: Deprecating BTK_WIDGET_SET_FLAGS requires fixing BTK internals. */

/**
 * BTK_WIDGET_UNSET_FLAGS:
 * @wid: a #BtkWidget.
 * @flag: the flags to unset.
 *
 * Turns off certain widget flags.
 *
 * Deprecated: 2.22: Use the proper function instead. See BTK_WIDGET_SET_FLAGS().
 */
#define BTK_WIDGET_UNSET_FLAGS(wid,flag)  B_STMT_START{ (BTK_WIDGET_FLAGS (wid) &= ~(flag)); }B_STMT_END
/* FIXME: Deprecating BTK_WIDGET_UNSET_FLAGS requires fixing BTK internals. */

#define BTK_TYPE_REQUISITION              (btk_requisition_get_type ())

/* forward declaration to avoid excessive includes (and concurrent includes)
 */
typedef struct _BtkRequisition	   BtkRequisition;
typedef struct _BtkSelectionData   BtkSelectionData;
typedef struct _BtkWidgetClass	   BtkWidgetClass;
typedef struct _BtkWidgetAuxInfo   BtkWidgetAuxInfo;
typedef struct _BtkWidgetShapeInfo BtkWidgetShapeInfo;
typedef struct _BtkClipboard	   BtkClipboard;
typedef struct _BtkTooltip         BtkTooltip;
typedef struct _BtkWindow          BtkWindow;

/**
 * BtkAllocation:
 * @x: the X position of the widget's area relative to its parents allocation.
 * @y: the Y position of the widget's area relative to its parents allocation.
 * @width: the width of the widget's allocated area.
 * @height: the height of the widget's allocated area.
 *
 * A <structname>BtkAllocation</structname> of a widget represents rebunnyion which has been allocated to the
 * widget by its parent. It is a subrebunnyion of its parents allocation. See
 * <xref linkend="size-allocation"/> for more information.
 */
typedef 	BdkRectangle	   BtkAllocation;

/**
 * BtkCallback:
 * @widget: the widget to operate on
 * @data: user-supplied data
 *
 * The type of the callback functions used for e.g. iterating over
 * the children of a container, see btk_container_foreach().
 */
typedef void    (*BtkCallback)     (BtkWidget        *widget,
				    gpointer          data);

/**
 * BtkRequisition:
 * @width: the widget's desired width
 * @height: the widget's desired height
 *
 * A <structname>BtkRequisition</structname> represents the desired size of a widget. See
 * <xref linkend="size-requisition"/> for more information.
 */
struct _BtkRequisition
{
  gint width;
  gint height;
};

/* The widget is the base of the tree for displayable objects.
 *  (A displayable object is one which takes up some amount
 *  of screen real estate). It provides a common base and interface
 *  which actual widgets must adhere to.
 */
struct _BtkWidget
{
  /* The object structure needs to be the first
   *  element in the widget structure in order for
   *  the object mechanism to work correctly. This
   *  allows a BtkWidget pointer to be cast to a
   *  BtkObject pointer.
   */
  BtkObject object;
  
  /* 16 bits of internally used private flags.
   * this will be packed into the same 4 byte alignment frame that
   * state and saved_state go. we therefore don't waste any new
   * space on this.
   */
  guint16 GSEAL (private_flags);
  
  /* The state of the widget. There are actually only
   *  5 widget states (defined in "btkenums.h").
   */
  guint8 GSEAL (state);
  
  /* The saved state of the widget. When a widget's state
   *  is changed to BTK_STATE_INSENSITIVE via
   *  "btk_widget_set_state" or "btk_widget_set_sensitive"
   *  the old state is kept around in this field. The state
   *  will be restored once the widget gets sensitive again.
   */
  guint8 GSEAL (saved_state);
  
  /* The widget's name. If the widget does not have a name
   *  (the name is NULL), then its name (as returned by
   *  "btk_widget_get_name") is its class's name.
   * Among other things, the widget name is used to determine
   *  the style to use for a widget.
   */
  gchar *GSEAL (name);
  
  /*< public >*/

  /* The style for the widget. The style contains the
   *  colors the widget should be drawn in for each state
   *  along with graphics contexts used to draw with and
   *  the font to use for text.
   */
  BtkStyle *GSEAL (style);
  
  /* The widget's desired size.
   */
  BtkRequisition GSEAL (requisition);
  
  /* The widget's allocated size.
   */
  BtkAllocation GSEAL (allocation);
  
  /* The widget's window or its parent window if it does
   *  not have a window. (Which will be indicated by the
   *  BTK_NO_WINDOW flag being set).
   */
  BdkWindow *GSEAL (window);
  
  /* The widget's parent.
   */
  BtkWidget *GSEAL (parent);
};

/**
 * BtkWidgetClass:
 * @parent_class:
 * @activate_signal:
 * @set_scroll_adjustments_signal:
 *
 * <structfield>activate_signal</structfield>
 * The signal to emit when a widget of this class is activated,
 * btk_widget_activate() handles the emission. Implementation of this
 * signal is optional.
 *
 *
 * <structfield>set_scroll_adjustment_signal</structfield>
 * This signal is emitted  when a widget of this class is added
 * to a scrolling aware parent, btk_widget_set_scroll_adjustments()
 * handles the emission.
 * Implementation of this signal is optional.
 */
struct _BtkWidgetClass
{
  /* The object class structure needs to be the first
   *  element in the widget class structure in order for
   *  the class mechanism to work correctly. This allows a
   *  BtkWidgetClass pointer to be cast to a BtkObjectClass
   *  pointer.
   */
  BtkObjectClass parent_class;

  /*< public >*/
  
  guint activate_signal;

  guint set_scroll_adjustments_signal;

  /*< private >*/
  
  /* seldomly overidden */
  void (*dispatch_child_properties_changed) (BtkWidget   *widget,
					     guint        n_pspecs,
					     GParamSpec **pspecs);

  /* basics */
  void (* show)		       (BtkWidget        *widget);
  void (* show_all)            (BtkWidget        *widget);
  void (* hide)		       (BtkWidget        *widget);
  void (* hide_all)            (BtkWidget        *widget);
  void (* map)		       (BtkWidget        *widget);
  void (* unmap)	       (BtkWidget        *widget);
  void (* realize)	       (BtkWidget        *widget);
  void (* unrealize)	       (BtkWidget        *widget);
  void (* size_request)	       (BtkWidget        *widget,
				BtkRequisition   *requisition);
  void (* size_allocate)       (BtkWidget        *widget,
				BtkAllocation    *allocation);
  void (* state_changed)       (BtkWidget        *widget,
				BtkStateType   	  previous_state);
  void (* parent_set)	       (BtkWidget        *widget,
				BtkWidget        *previous_parent);
  void (* hierarchy_changed)   (BtkWidget        *widget,
				BtkWidget        *previous_toplevel);
  void (* style_set)	       (BtkWidget        *widget,
				BtkStyle         *previous_style);
  void (* direction_changed)   (BtkWidget        *widget,
				BtkTextDirection  previous_direction);
  void (* grab_notify)         (BtkWidget        *widget,
				gboolean          was_grabbed);
  void (* child_notify)        (BtkWidget	 *widget,
				GParamSpec       *pspec);
  
  /* Mnemonics */
  gboolean (* mnemonic_activate) (BtkWidget    *widget,
				  gboolean      group_cycling);
  
  /* explicit focus */
  void     (* grab_focus)      (BtkWidget        *widget);
  gboolean (* focus)           (BtkWidget        *widget,
                                BtkDirectionType  direction);
  
  /* events */
  gboolean (* event)			(BtkWidget	     *widget,
					 BdkEvent	     *event);
  gboolean (* button_press_event)	(BtkWidget	     *widget,
					 BdkEventButton      *event);
  gboolean (* button_release_event)	(BtkWidget	     *widget,
					 BdkEventButton      *event);
  gboolean (* scroll_event)		(BtkWidget           *widget,
					 BdkEventScroll      *event);
  gboolean (* motion_notify_event)	(BtkWidget	     *widget,
					 BdkEventMotion      *event);
  gboolean (* delete_event)		(BtkWidget	     *widget,
					 BdkEventAny	     *event);
  gboolean (* destroy_event)		(BtkWidget	     *widget,
					 BdkEventAny	     *event);
  gboolean (* expose_event)		(BtkWidget	     *widget,
					 BdkEventExpose      *event);
  gboolean (* key_press_event)		(BtkWidget	     *widget,
					 BdkEventKey	     *event);
  gboolean (* key_release_event)	(BtkWidget	     *widget,
					 BdkEventKey	     *event);
  gboolean (* enter_notify_event)	(BtkWidget	     *widget,
					 BdkEventCrossing    *event);
  gboolean (* leave_notify_event)	(BtkWidget	     *widget,
					 BdkEventCrossing    *event);
  gboolean (* configure_event)		(BtkWidget	     *widget,
					 BdkEventConfigure   *event);
  gboolean (* focus_in_event)		(BtkWidget	     *widget,
					 BdkEventFocus       *event);
  gboolean (* focus_out_event)		(BtkWidget	     *widget,
					 BdkEventFocus       *event);
  gboolean (* map_event)		(BtkWidget	     *widget,
					 BdkEventAny	     *event);
  gboolean (* unmap_event)		(BtkWidget	     *widget,
					 BdkEventAny	     *event);
  gboolean (* property_notify_event)	(BtkWidget	     *widget,
					 BdkEventProperty    *event);
  gboolean (* selection_clear_event)	(BtkWidget	     *widget,
					 BdkEventSelection   *event);
  gboolean (* selection_request_event)	(BtkWidget	     *widget,
					 BdkEventSelection   *event);
  gboolean (* selection_notify_event)	(BtkWidget	     *widget,
					 BdkEventSelection   *event);
  gboolean (* proximity_in_event)	(BtkWidget	     *widget,
					 BdkEventProximity   *event);
  gboolean (* proximity_out_event)	(BtkWidget	     *widget,
					 BdkEventProximity   *event);
  gboolean (* visibility_notify_event)	(BtkWidget	     *widget,
					 BdkEventVisibility  *event);
  gboolean (* client_event)		(BtkWidget	     *widget,
					 BdkEventClient	     *event);
  gboolean (* no_expose_event)		(BtkWidget	     *widget,
					 BdkEventAny	     *event);
  gboolean (* window_state_event)	(BtkWidget	     *widget,
					 BdkEventWindowState *event);
  
  /* selection */
  void (* selection_get)           (BtkWidget          *widget,
				    BtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  void (* selection_received)      (BtkWidget          *widget,
				    BtkSelectionData   *selection_data,
				    guint               time_);

  /* Source side drag signals */
  void (* drag_begin)	           (BtkWidget	       *widget,
				    BdkDragContext     *context);
  void (* drag_end)	           (BtkWidget	       *widget,
				    BdkDragContext     *context);
  void (* drag_data_get)           (BtkWidget          *widget,
				    BdkDragContext     *context,
				    BtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);
  void (* drag_data_delete)        (BtkWidget	       *widget,
				    BdkDragContext     *context);

  /* Target side drag signals */
  void (* drag_leave)	           (BtkWidget	       *widget,
				    BdkDragContext     *context,
				    guint               time_);
  gboolean (* drag_motion)         (BtkWidget	       *widget,
				    BdkDragContext     *context,
				    gint                x,
				    gint                y,
				    guint               time_);
  gboolean (* drag_drop)           (BtkWidget	       *widget,
				    BdkDragContext     *context,
				    gint                x,
				    gint                y,
				    guint               time_);
  void (* drag_data_received)      (BtkWidget          *widget,
				    BdkDragContext     *context,
				    gint                x,
				    gint                y,
				    BtkSelectionData   *selection_data,
				    guint               info,
				    guint               time_);

  /* Signals used only for keybindings */
  gboolean (* popup_menu)          (BtkWidget          *widget);

  /* If a widget has multiple tooltips/whatsthis, it should show the
   * one for the current focus location, or if that doesn't make
   * sense, should cycle through them showing each tip alongside
   * whatever piece of the widget it applies to.
   */
  gboolean (* show_help)           (BtkWidget          *widget,
                                    BtkWidgetHelpType   help_type);
  
  /* accessibility support 
   */
  BatkObject*   (*get_accessible)     (BtkWidget *widget);

  void         (*screen_changed)     (BtkWidget *widget,
                                      BdkScreen *previous_screen);
  gboolean     (*can_activate_accel) (BtkWidget *widget,
                                      guint      signal_id);

  /* Sent when a grab is broken. */
  gboolean (*grab_broken_event) (BtkWidget	     *widget,
                                 BdkEventGrabBroken  *event);

  void         (* composited_changed) (BtkWidget *widget);

  gboolean     (* query_tooltip)      (BtkWidget  *widget,
				       gint        x,
				       gint        y,
				       gboolean    keyboard_tooltip,
				       BtkTooltip *tooltip);
  /* Signals without a C default handler class slot:
   * gboolean	(*damage_event)	(BtkWidget      *widget,
   *                             BdkEventExpose *event);
   */

  /* Padding for future expansion */
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
  void (*_btk_reserved7) (void);
};

struct _BtkWidgetAuxInfo
{
  gint x;
  gint y;
  gint width;
  gint height;
  guint x_set : 1;
  guint y_set : 1;
};

struct _BtkWidgetShapeInfo
{
  gint16     offset_x;
  gint16     offset_y;
  BdkBitmap *shape_mask;
};

GType	   btk_widget_get_type		  (void) B_GNUC_CONST;
BtkWidget* btk_widget_new		  (GType		type,
					   const gchar	       *first_property_name,
					   ...);
void	   btk_widget_destroy		  (BtkWidget	       *widget);
void	   btk_widget_destroyed		  (BtkWidget	       *widget,
					   BtkWidget	      **widget_pointer);
#ifndef BTK_DISABLE_DEPRECATED
BtkWidget* btk_widget_ref		  (BtkWidget	       *widget);
void	   btk_widget_unref		  (BtkWidget	       *widget);
void	   btk_widget_set		  (BtkWidget	       *widget,
					   const gchar         *first_property_name,
					   ...) B_GNUC_NULL_TERMINATED;
#endif /* BTK_DISABLE_DEPRECATED */
#if !defined(BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
void       btk_widget_hide_all            (BtkWidget           *widget);
#endif
void	   btk_widget_unparent		  (BtkWidget	       *widget);
void	   btk_widget_show		  (BtkWidget	       *widget);
void       btk_widget_show_now            (BtkWidget           *widget);
void	   btk_widget_hide		  (BtkWidget	       *widget);
void	   btk_widget_show_all		  (BtkWidget	       *widget);
void       btk_widget_set_no_show_all     (BtkWidget           *widget,
					   gboolean             no_show_all);
gboolean   btk_widget_get_no_show_all     (BtkWidget           *widget);
void	   btk_widget_map		  (BtkWidget	       *widget);
void	   btk_widget_unmap		  (BtkWidget	       *widget);
void	   btk_widget_realize		  (BtkWidget	       *widget);
void	   btk_widget_unrealize		  (BtkWidget	       *widget);

/* Queuing draws */
void	   btk_widget_queue_draw	  (BtkWidget	       *widget);
void	   btk_widget_queue_draw_area	  (BtkWidget	       *widget,
					   gint                 x,
					   gint                 y,
					   gint                 width,
					   gint                 height);
#ifndef BTK_DISABLE_DEPRECATED
void	   btk_widget_queue_clear	  (BtkWidget	       *widget);
void	   btk_widget_queue_clear_area	  (BtkWidget	       *widget,
					   gint                 x,
					   gint                 y,
					   gint                 width,
					   gint                 height);
#endif /* BTK_DISABLE_DEPRECATED */


void	   btk_widget_queue_resize	  (BtkWidget	       *widget);
void	   btk_widget_queue_resize_no_redraw (BtkWidget *widget);
#ifndef BTK_DISABLE_DEPRECATED
void	   btk_widget_draw		  (BtkWidget	       *widget,
					   const BdkRectangle  *area);
#endif /* BTK_DISABLE_DEPRECATED */
void	   btk_widget_size_request	  (BtkWidget	       *widget,
					   BtkRequisition      *requisition);
void	   btk_widget_size_allocate	  (BtkWidget	       *widget,
					   BtkAllocation       *allocation);
void       btk_widget_get_child_requisition (BtkWidget	       *widget,
					     BtkRequisition    *requisition);
void	   btk_widget_add_accelerator	  (BtkWidget           *widget,
					   const gchar         *accel_signal,
					   BtkAccelGroup       *accel_group,
					   guint                accel_key,
					   BdkModifierType      accel_mods,
					   BtkAccelFlags        accel_flags);
gboolean   btk_widget_remove_accelerator  (BtkWidget           *widget,
					   BtkAccelGroup       *accel_group,
					   guint                accel_key,
					   BdkModifierType      accel_mods);
void       btk_widget_set_accel_path      (BtkWidget           *widget,
					   const gchar         *accel_path,
					   BtkAccelGroup       *accel_group);
const gchar* _btk_widget_get_accel_path   (BtkWidget           *widget,
					   gboolean	       *locked);
GList*     btk_widget_list_accel_closures (BtkWidget	       *widget);
gboolean   btk_widget_can_activate_accel  (BtkWidget           *widget,
                                           guint                signal_id);
gboolean   btk_widget_mnemonic_activate   (BtkWidget           *widget,
					   gboolean             group_cycling);
gboolean   btk_widget_event		  (BtkWidget	       *widget,
					   BdkEvent	       *event);
gint       btk_widget_send_expose         (BtkWidget           *widget,
					   BdkEvent            *event);
gboolean   btk_widget_send_focus_change   (BtkWidget           *widget,
                                           BdkEvent            *event);

gboolean   btk_widget_activate		     (BtkWidget	       *widget);
gboolean   btk_widget_set_scroll_adjustments (BtkWidget        *widget,
					      BtkAdjustment    *hadjustment,
					      BtkAdjustment    *vadjustment);
     
void	   btk_widget_reparent		  (BtkWidget	       *widget,
					   BtkWidget	       *new_parent);
gboolean   btk_widget_intersect		  (BtkWidget	       *widget,
					   const BdkRectangle  *area,
					   BdkRectangle	       *intersection);
BdkRebunnyion *btk_widget_rebunnyion_intersect	  (BtkWidget	       *widget,
					   const BdkRebunnyion     *rebunnyion);

void	btk_widget_freeze_child_notify	  (BtkWidget	       *widget);
void	btk_widget_child_notify		  (BtkWidget	       *widget,
					   const gchar	       *child_property);
void	btk_widget_thaw_child_notify	  (BtkWidget	       *widget);

void       btk_widget_set_can_focus       (BtkWidget           *widget,
                                           gboolean             can_focus);
gboolean   btk_widget_get_can_focus       (BtkWidget           *widget);
gboolean   btk_widget_has_focus           (BtkWidget           *widget);
gboolean   btk_widget_is_focus            (BtkWidget           *widget);
void       btk_widget_grab_focus          (BtkWidget           *widget);

void       btk_widget_set_can_default     (BtkWidget           *widget,
                                           gboolean             can_default);
gboolean   btk_widget_get_can_default     (BtkWidget           *widget);
gboolean   btk_widget_has_default         (BtkWidget           *widget);
void       btk_widget_grab_default        (BtkWidget           *widget);

void      btk_widget_set_receives_default (BtkWidget           *widget,
                                           gboolean             receives_default);
gboolean  btk_widget_get_receives_default (BtkWidget           *widget);

gboolean   btk_widget_has_grab            (BtkWidget           *widget);

void                  btk_widget_set_name               (BtkWidget    *widget,
							 const gchar  *name);
const gchar*          btk_widget_get_name               (BtkWidget    *widget);

void                  btk_widget_set_state              (BtkWidget    *widget,
							 BtkStateType  state);
BtkStateType          btk_widget_get_state              (BtkWidget    *widget);

void                  btk_widget_set_sensitive          (BtkWidget    *widget,
							 gboolean      sensitive);
gboolean              btk_widget_get_sensitive          (BtkWidget    *widget);
gboolean              btk_widget_is_sensitive           (BtkWidget    *widget);

void                  btk_widget_set_visible            (BtkWidget    *widget,
                                                         gboolean      visible);
gboolean              btk_widget_get_visible            (BtkWidget    *widget);

void                  btk_widget_set_has_window         (BtkWidget    *widget,
                                                         gboolean      has_window);
gboolean              btk_widget_get_has_window         (BtkWidget    *widget);

gboolean              btk_widget_is_toplevel            (BtkWidget    *widget);
gboolean              btk_widget_is_drawable            (BtkWidget    *widget);
void                  btk_widget_set_realized           (BtkWidget    *widget,
                                                         gboolean      realized);
gboolean              btk_widget_get_realized           (BtkWidget    *widget);
void                  btk_widget_set_mapped             (BtkWidget    *widget,
                                                         gboolean      mapped);
gboolean              btk_widget_get_mapped             (BtkWidget    *widget);

void                  btk_widget_set_app_paintable      (BtkWidget    *widget,
							 gboolean      app_paintable);
gboolean              btk_widget_get_app_paintable      (BtkWidget    *widget);

void                  btk_widget_set_double_buffered    (BtkWidget    *widget,
							 gboolean      double_buffered);
gboolean              btk_widget_get_double_buffered    (BtkWidget    *widget);

void                  btk_widget_set_redraw_on_allocate (BtkWidget    *widget,
							 gboolean      redraw_on_allocate);

void                  btk_widget_set_parent             (BtkWidget    *widget,
							 BtkWidget    *parent);
BtkWidget           * btk_widget_get_parent             (BtkWidget    *widget);

void                  btk_widget_set_parent_window      (BtkWidget    *widget,
							 BdkWindow    *parent_window);
BdkWindow           * btk_widget_get_parent_window      (BtkWidget    *widget);

void                  btk_widget_set_child_visible      (BtkWidget    *widget,
							 gboolean      is_visible);
gboolean              btk_widget_get_child_visible      (BtkWidget    *widget);

void                  btk_widget_set_window             (BtkWidget    *widget,
                                                         BdkWindow    *window);
BdkWindow           * btk_widget_get_window             (BtkWidget    *widget);

void                  btk_widget_get_allocation         (BtkWidget     *widget,
                                                         BtkAllocation *allocation);
void                  btk_widget_set_allocation         (BtkWidget     *widget,
                                                         const BtkAllocation *allocation);

void                  btk_widget_get_requisition        (BtkWidget     *widget,
                                                         BtkRequisition *requisition);

gboolean   btk_widget_child_focus         (BtkWidget           *widget,
                                           BtkDirectionType     direction);
gboolean   btk_widget_keynav_failed       (BtkWidget           *widget,
                                           BtkDirectionType     direction);
void       btk_widget_error_bell          (BtkWidget           *widget);

void       btk_widget_set_size_request    (BtkWidget           *widget,
                                           gint                 width,
                                           gint                 height);
void       btk_widget_get_size_request    (BtkWidget           *widget,
                                           gint                *width,
                                           gint                *height);
#ifndef BTK_DISABLE_DEPRECATED
void	   btk_widget_set_uposition	  (BtkWidget	       *widget,
					   gint			x,
					   gint			y);
void	   btk_widget_set_usize		  (BtkWidget	       *widget,
					   gint			width,
					   gint			height);
#endif

void	   btk_widget_set_events	  (BtkWidget	       *widget,
					   gint			events);
void       btk_widget_add_events          (BtkWidget           *widget,
					   gint	                events);
void	   btk_widget_set_extension_events (BtkWidget		*widget,
					    BdkExtensionMode	mode);

BdkExtensionMode btk_widget_get_extension_events (BtkWidget	*widget);
BtkWidget*   btk_widget_get_toplevel	(BtkWidget	*widget);
BtkWidget*   btk_widget_get_ancestor	(BtkWidget	*widget,
					 GType		 widget_type);
BdkColormap* btk_widget_get_colormap	(BtkWidget	*widget);
BdkVisual*   btk_widget_get_visual	(BtkWidget	*widget);

BdkScreen *   btk_widget_get_screen      (BtkWidget *widget);
gboolean      btk_widget_has_screen      (BtkWidget *widget);
BdkDisplay *  btk_widget_get_display     (BtkWidget *widget);
BdkWindow *   btk_widget_get_root_window (BtkWidget *widget);
BtkSettings*  btk_widget_get_settings    (BtkWidget *widget);
BtkClipboard *btk_widget_get_clipboard   (BtkWidget *widget,
					  BdkAtom    selection);
BdkPixmap *   btk_widget_get_snapshot    (BtkWidget    *widget,
                                          BdkRectangle *clip_rect);

#ifndef BTK_DISABLE_DEPRECATED

/**
 * btk_widget_set_visual:
 * @widget: a #BtkWidget
 * @visual: a visual
 *
 * This function is deprecated; it does nothing.
 */
#define btk_widget_set_visual(widget,visual)  ((void) 0)

/**
 * btk_widget_push_visual:
 * @visual: a visual
 *
 * This function is deprecated; it does nothing.
 */
#define btk_widget_push_visual(visual)        ((void) 0)

/**
 * btk_widget_pop_visual:
 *
 * This function is deprecated; it does nothing.
 */
#define btk_widget_pop_visual()               ((void) 0)

/**
 * btk_widget_set_default_visual:
 * @visual: a visual
 *
 * This function is deprecated; it does nothing.
 */
#define btk_widget_set_default_visual(visual) ((void) 0)

#endif /* BTK_DISABLE_DEPRECATED */

/* Accessibility support */
BatkObject*       btk_widget_get_accessible               (BtkWidget          *widget);

/* The following functions must not be called on an already
 * realized widget. Because it is possible that somebody
 * can call get_colormap() or get_visual() and save the
 * result, these functions are probably only safe to
 * call in a widget's init() function.
 */
void         btk_widget_set_colormap    (BtkWidget      *widget,
					 BdkColormap    *colormap);

gint	     btk_widget_get_events	(BtkWidget	*widget);
void	     btk_widget_get_pointer	(BtkWidget	*widget,
					 gint		*x,
					 gint		*y);

gboolean     btk_widget_is_ancestor	(BtkWidget	*widget,
					 BtkWidget	*ancestor);

gboolean     btk_widget_translate_coordinates (BtkWidget  *src_widget,
					       BtkWidget  *dest_widget,
					       gint        src_x,
					       gint        src_y,
					       gint       *dest_x,
					       gint       *dest_y);

/* Hide widget and return TRUE.
 */
gboolean     btk_widget_hide_on_delete	(BtkWidget	*widget);

/* Widget styles.
 */
void        btk_widget_style_attach       (BtkWidget            *style);

gboolean    btk_widget_has_rc_style       (BtkWidget            *widget);
void	    btk_widget_set_style          (BtkWidget            *widget,
                                           BtkStyle             *style);
void        btk_widget_ensure_style       (BtkWidget            *widget);
BtkStyle *  btk_widget_get_style          (BtkWidget            *widget);

void        btk_widget_modify_style       (BtkWidget            *widget,
					   BtkRcStyle           *style);
BtkRcStyle *btk_widget_get_modifier_style (BtkWidget            *widget);
void        btk_widget_modify_fg          (BtkWidget            *widget,
					   BtkStateType          state,
					   const BdkColor       *color);
void        btk_widget_modify_bg          (BtkWidget            *widget,
					   BtkStateType          state,
					   const BdkColor       *color);
void        btk_widget_modify_text        (BtkWidget            *widget,
					   BtkStateType          state,
					   const BdkColor       *color);
void        btk_widget_modify_base        (BtkWidget            *widget,
					   BtkStateType          state,
					   const BdkColor       *color);
void        btk_widget_modify_cursor      (BtkWidget            *widget,
					   const BdkColor       *primary,
					   const BdkColor       *secondary);
void        btk_widget_modify_font        (BtkWidget            *widget,
					   BangoFontDescription *font_desc);

#ifndef BTK_DISABLE_DEPRECATED

/**
 * btk_widget_set_rc_style:
 * @widget: a #BtkWidget.
 *
 * Equivalent to <literal>btk_widget_set_style (widget, NULL)</literal>.
 *
 * Deprecated: 2.0: Use btk_widget_set_style() with a %NULL @style argument instead.
 */
#define btk_widget_set_rc_style(widget)          (btk_widget_set_style (widget, NULL))

/**
 * btk_widget_restore_default_style:
 * @widget: a #BtkWidget.
 *
 * Equivalent to <literal>btk_widget_set_style (widget, NULL)</literal>.
 *
 * Deprecated: 2.0: Use btk_widget_set_style() with a %NULL @style argument instead.
 */
#define btk_widget_restore_default_style(widget) (btk_widget_set_style (widget, NULL))
#endif

BangoContext *btk_widget_create_bango_context (BtkWidget   *widget);
BangoContext *btk_widget_get_bango_context    (BtkWidget   *widget);
BangoLayout  *btk_widget_create_bango_layout  (BtkWidget   *widget,
					       const gchar *text);

BdkPixbuf    *btk_widget_render_icon          (BtkWidget   *widget,
                                               const gchar *stock_id,
                                               BtkIconSize  size,
                                               const gchar *detail);

/* handle composite names for BTK_COMPOSITE_CHILD widgets,
 * the returned name is newly allocated.
 */
void   btk_widget_set_composite_name	(BtkWidget	*widget,
					 const gchar   	*name);
gchar* btk_widget_get_composite_name	(BtkWidget	*widget);
     
/* Descend recursively and set rc-style on all widgets without user styles */
void       btk_widget_reset_rc_styles   (BtkWidget      *widget);

/* Push/pop pairs, to change default values upon a widget's creation.
 * This will override the values that got set by the
 * btk_widget_set_default_* () functions.
 */
void	     btk_widget_push_colormap	     (BdkColormap *cmap);
void	     btk_widget_push_composite_child (void);
void	     btk_widget_pop_composite_child  (void);
void	     btk_widget_pop_colormap	     (void);

/* widget style properties
 */
void btk_widget_class_install_style_property        (BtkWidgetClass     *klass,
						     GParamSpec         *pspec);
void btk_widget_class_install_style_property_parser (BtkWidgetClass     *klass,
						     GParamSpec         *pspec,
						     BtkRcPropertyParser parser);
GParamSpec*  btk_widget_class_find_style_property   (BtkWidgetClass     *klass,
						     const gchar        *property_name);
GParamSpec** btk_widget_class_list_style_properties (BtkWidgetClass     *klass,
						     guint              *n_properties);
void btk_widget_style_get_property (BtkWidget	     *widget,
				    const gchar    *property_name,
				    GValue	     *value);
void btk_widget_style_get_valist   (BtkWidget	     *widget,
				    const gchar    *first_property_name,
				    va_list         var_args);
void btk_widget_style_get          (BtkWidget	     *widget,
				    const gchar    *first_property_name,
				    ...) B_GNUC_NULL_TERMINATED;


/* Set certain default values to be used at widget creation time.
 */
void	     btk_widget_set_default_colormap (BdkColormap *colormap);
BtkStyle*    btk_widget_get_default_style    (void);
#ifndef BDK_MULTIHEAD_SAFE
BdkColormap* btk_widget_get_default_colormap (void);
BdkVisual*   btk_widget_get_default_visual   (void);
#endif

/* Functions for setting directionality for widgets
 */

void             btk_widget_set_direction         (BtkWidget        *widget,
						   BtkTextDirection  dir);
BtkTextDirection btk_widget_get_direction         (BtkWidget        *widget);

void             btk_widget_set_default_direction (BtkTextDirection  dir);
BtkTextDirection btk_widget_get_default_direction (void);

/* Compositing manager functionality */
gboolean btk_widget_is_composited (BtkWidget *widget);

/* Counterpart to bdk_window_shape_combine_mask.
 */
void	     btk_widget_shape_combine_mask (BtkWidget *widget,
					    BdkBitmap *shape_mask,
					    gint       offset_x,
					    gint       offset_y);
void	     btk_widget_input_shape_combine_mask (BtkWidget *widget,
						  BdkBitmap *shape_mask,
						  gint       offset_x,
						  gint       offset_y);

#if !defined(BTK_DISABLE_DEPRECATED) || defined (BTK_COMPILATION)
/* internal function */
void	     btk_widget_reset_shapes	   (BtkWidget *widget);
#endif

/* Compute a widget's path in the form "BtkWindow.MyLabel", and
 * return newly alocated strings.
 */
void	     btk_widget_path		   (BtkWidget *widget,
					    guint     *path_length,
					    gchar    **path,
					    gchar    **path_reversed);
void	     btk_widget_class_path	   (BtkWidget *widget,
					    guint     *path_length,
					    gchar    **path,
					    gchar    **path_reversed);

GList* btk_widget_list_mnemonic_labels  (BtkWidget *widget);
void   btk_widget_add_mnemonic_label    (BtkWidget *widget,
					 BtkWidget *label);
void   btk_widget_remove_mnemonic_label (BtkWidget *widget,
					 BtkWidget *label);

void                  btk_widget_set_tooltip_window    (BtkWidget   *widget,
                                                        BtkWindow   *custom_window);
BtkWindow *btk_widget_get_tooltip_window    (BtkWidget   *widget);
void       btk_widget_trigger_tooltip_query (BtkWidget   *widget);
void       btk_widget_set_tooltip_text      (BtkWidget   *widget,
                                             const gchar *text);
gchar *    btk_widget_get_tooltip_text      (BtkWidget   *widget);
void       btk_widget_set_tooltip_markup    (BtkWidget   *widget,
                                             const gchar *markup);
gchar *    btk_widget_get_tooltip_markup    (BtkWidget   *widget);
void       btk_widget_set_has_tooltip       (BtkWidget   *widget,
					     gboolean     has_tooltip);
gboolean   btk_widget_get_has_tooltip       (BtkWidget   *widget);

GType           btk_requisition_get_type (void) B_GNUC_CONST;
BtkRequisition *btk_requisition_copy     (const BtkRequisition *requisition);
void            btk_requisition_free     (BtkRequisition       *requisition);

#if	defined (BTK_TRACE_OBJECTS) && defined (__GNUC__)
#  define btk_widget_ref g_object_ref
#  define btk_widget_unref g_object_unref
#endif	/* BTK_TRACE_OBJECTS && __GNUC__ */

void              _btk_widget_set_has_default             (BtkWidget    *widget,
                                                           gboolean      has_default);
void              _btk_widget_set_has_grab                (BtkWidget    *widget,
                                                           gboolean      has_grab);
void              _btk_widget_set_is_toplevel             (BtkWidget    *widget,
                                                           gboolean      is_toplevel);

void              _btk_widget_grab_notify                 (BtkWidget    *widget,
						           gboolean	was_grabbed);

BtkWidgetAuxInfo *_btk_widget_get_aux_info                (BtkWidget    *widget,
							   gboolean      create);
void              _btk_widget_propagate_hierarchy_changed (BtkWidget    *widget,
							   BtkWidget    *previous_toplevel);
void              _btk_widget_propagate_screen_changed    (BtkWidget    *widget,
							   BdkScreen    *previous_screen);
void		  _btk_widget_propagate_composited_changed (BtkWidget    *widget);

void	   _btk_widget_set_pointer_window  (BtkWidget      *widget,
					    BdkWindow      *pointer_window);
BdkWindow *_btk_widget_get_pointer_window  (BtkWidget      *widget);
gboolean   _btk_widget_is_pointer_widget   (BtkWidget      *widget);
void       _btk_widget_synthesize_crossing (BtkWidget      *from,
					    BtkWidget      *to,
					    BdkCrossingMode mode);

BdkColormap* _btk_widget_peek_colormap (void);

void         _btk_widget_buildable_finish_accelerator (BtkWidget *widget,
						       BtkWidget *toplevel,
						       gpointer   user_data);

B_END_DECLS

#endif /* __BTK_WIDGET_H__ */
