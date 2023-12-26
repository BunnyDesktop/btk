/* btkcellrenderer.c
 * Copyright (C) 2000  Red Hat, Inc. Jonathan Blandford
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

#include "config.h"
#include "btkcellrenderer.h"
#include "btkintl.h"
#include "btkmarshalers.h"
#include "btkprivate.h"
#include "btktreeprivate.h"
#include "btkalias.h"

static void btk_cell_renderer_get_property  (BObject              *object,
					     buint                 param_id,
					     BValue               *value,
					     BParamSpec           *pspec);
static void btk_cell_renderer_set_property  (BObject              *object,
					     buint                 param_id,
					     const BValue         *value,
					     BParamSpec           *pspec);
static void set_cell_bg_color               (BtkCellRenderer      *cell,
					     BdkColor             *color);


#define BTK_CELL_RENDERER_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_CELL_RENDERER, BtkCellRendererPrivate))

typedef struct _BtkCellRendererPrivate BtkCellRendererPrivate;
struct _BtkCellRendererPrivate
{
  BdkColor cell_background;
};


enum {
  PROP_0,
  PROP_MODE,
  PROP_VISIBLE,
  PROP_SENSITIVE,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_XPAD,
  PROP_YPAD,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_IS_EXPANDER,
  PROP_IS_EXPANDED,
  PROP_CELL_BACKGROUND,
  PROP_CELL_BACKGROUND_BDK,
  PROP_CELL_BACKGROUND_SET,
  PROP_EDITING
};

/* Signal IDs */
enum {
  EDITING_CANCELED,
  EDITING_STARTED,
  LAST_SIGNAL
};

static buint cell_renderer_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (BtkCellRenderer, btk_cell_renderer, BTK_TYPE_OBJECT)

static void
btk_cell_renderer_init (BtkCellRenderer *cell)
{
  cell->mode = BTK_CELL_RENDERER_MODE_INERT;
  cell->visible = TRUE;
  cell->width = -1;
  cell->height = -1;
  cell->xalign = 0.5;
  cell->yalign = 0.5;
  cell->xpad = 0;
  cell->ypad = 0;
  cell->sensitive = TRUE;
  cell->is_expander = FALSE;
  cell->is_expanded = FALSE;
  cell->editing = FALSE;
}

static void
btk_cell_renderer_class_init (BtkCellRendererClass *class)
{
  BObjectClass *object_class = B_OBJECT_CLASS (class);

  object_class->get_property = btk_cell_renderer_get_property;
  object_class->set_property = btk_cell_renderer_set_property;

  class->render = NULL;
  class->get_size = NULL;

  /**
   * BtkCellRenderer::editing-canceled:
   * @renderer: the object which received the signal
   *
   * This signal gets emitted when the user cancels the process of editing a
   * cell.  For example, an editable cell renderer could be written to cancel
   * editing when the user presses Escape. 
   *
   * See also: btk_cell_renderer_stop_editing().
   *
   * Since: 2.4
   */
  cell_renderer_signals[EDITING_CANCELED] =
    g_signal_new (I_("editing-canceled"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCellRendererClass, editing_canceled),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  B_TYPE_NONE, 0);

  /**
   * BtkCellRenderer::editing-started:
   * @renderer: the object which received the signal
   * @editable: the #BtkCellEditable
   * @path: the path identifying the edited cell
   *
   * This signal gets emitted when a cell starts to be edited.
   * The intended use of this signal is to do special setup
   * on @editable, e.g. adding a #BtkEntryCompletion or setting
   * up additional columns in a #BtkComboBox.
   *
   * Note that BTK+ doesn't guarantee that cell renderers will
   * continue to use the same kind of widget for editing in future
   * releases, therefore you should check the type of @editable
   * before doing any specific setup, as in the following example:
   * |[
   * static void
   * text_editing_started (BtkCellRenderer *cell,
   *                       BtkCellEditable *editable,
   *                       const bchar     *path,
   *                       bpointer         data)
   * {
   *   if (BTK_IS_ENTRY (editable)) 
   *     {
   *       BtkEntry *entry = BTK_ENTRY (editable);
   *       
   *       /&ast; ... create a BtkEntryCompletion &ast;/
   *       
   *       btk_entry_set_completion (entry, completion);
   *     }
   * }
   * ]|
   *
   * Since: 2.6
   */
  cell_renderer_signals[EDITING_STARTED] =
    g_signal_new (I_("editing-started"),
		  B_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkCellRendererClass, editing_started),
		  NULL, NULL,
		  _btk_marshal_VOID__OBJECT_STRING,
		  B_TYPE_NONE, 2,
		  BTK_TYPE_CELL_EDITABLE,
		  B_TYPE_STRING);

  g_object_class_install_property (object_class,
				   PROP_MODE,
				   g_param_spec_enum ("mode",
						      P_("mode"),
						      P_("Editable mode of the CellRenderer"),
						      BTK_TYPE_CELL_RENDERER_MODE,
						      BTK_CELL_RENDERER_MODE_INERT,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible",
							 P_("visible"),
							 P_("Display the cell"),
							 TRUE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive",
							 P_("Sensitive"),
							 P_("Display the cell sensitive"),
							 TRUE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_XALIGN,
				   g_param_spec_float ("xalign",
						       P_("xalign"),
						       P_("The x-align"),
						       0.0,
						       1.0,
						       0.5,
						       BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_YALIGN,
				   g_param_spec_float ("yalign",
						       P_("yalign"),
						       P_("The y-align"),
						       0.0,
						       1.0,
						       0.5,
						       BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_XPAD,
				   g_param_spec_uint ("xpad",
						      P_("xpad"),
						      P_("The xpad"),
						      0,
						      B_MAXUINT,
						      0,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_YPAD,
				   g_param_spec_uint ("ypad",
						      P_("ypad"),
						      P_("The ypad"),
						      0,
						      B_MAXUINT,
						      0,
						      BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_WIDTH,
				   g_param_spec_int ("width",
						     P_("width"),
						     P_("The fixed width"),
						     -1,
						     B_MAXINT,
						     -1,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_HEIGHT,
				   g_param_spec_int ("height",
						     P_("height"),
						     P_("The fixed height"),
						     -1,
						     B_MAXINT,
						     -1,
						     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_IS_EXPANDER,
				   g_param_spec_boolean ("is-expander",
							 P_("Is Expander"),
							 P_("Row has children"),
							 FALSE,
							 BTK_PARAM_READWRITE));


  g_object_class_install_property (object_class,
				   PROP_IS_EXPANDED,
				   g_param_spec_boolean ("is-expanded",
							 P_("Is Expanded"),
							 P_("Row is an expander row, and is expanded"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND,
				   g_param_spec_string ("cell-background",
							P_("Cell background color name"),
							P_("Cell background color as a string"),
							NULL,
							BTK_PARAM_WRITABLE));

  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND_BDK,
				   g_param_spec_boxed ("cell-background-bdk",
						       P_("Cell background color"),
						       P_("Cell background color as a BdkColor"),
						       BDK_TYPE_COLOR,
						       BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_EDITING,
				   g_param_spec_boolean ("editing",
							 P_("Editing"),
							 P_("Whether the cell renderer is currently in editing mode"),
							 FALSE,
							 BTK_PARAM_READABLE));


#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (object_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, BTK_PARAM_READWRITE))

  ADD_SET_PROP ("cell-background-set", PROP_CELL_BACKGROUND_SET,
                P_("Cell background set"),
                P_("Whether this tag affects the cell background color"));

  g_type_class_add_private (object_class, sizeof (BtkCellRendererPrivate));
}

static void
btk_cell_renderer_get_property (BObject     *object,
				buint        param_id,
				BValue      *value,
				BParamSpec  *pspec)
{
  BtkCellRenderer *cell = BTK_CELL_RENDERER (object);
  BtkCellRendererPrivate *priv = BTK_CELL_RENDERER_GET_PRIVATE (object);

  switch (param_id)
    {
    case PROP_MODE:
      b_value_set_enum (value, cell->mode);
      break;
    case PROP_VISIBLE:
      b_value_set_boolean (value, cell->visible);
      break;
    case PROP_SENSITIVE:
      b_value_set_boolean (value, cell->sensitive);
      break;
    case PROP_EDITING:
      b_value_set_boolean (value, cell->editing);
      break;
    case PROP_XALIGN:
      b_value_set_float (value, cell->xalign);
      break;
    case PROP_YALIGN:
      b_value_set_float (value, cell->yalign);
      break;
    case PROP_XPAD:
      b_value_set_uint (value, cell->xpad);
      break;
    case PROP_YPAD:
      b_value_set_uint (value, cell->ypad);
      break;
    case PROP_WIDTH:
      b_value_set_int (value, cell->width);
      break;
    case PROP_HEIGHT:
      b_value_set_int (value, cell->height);
      break;
    case PROP_IS_EXPANDER:
      b_value_set_boolean (value, cell->is_expander);
      break;
    case PROP_IS_EXPANDED:
      b_value_set_boolean (value, cell->is_expanded);
      break;
    case PROP_CELL_BACKGROUND_BDK:
      {
	BdkColor color;

	color.red = priv->cell_background.red;
	color.green = priv->cell_background.green;
	color.blue = priv->cell_background.blue;

	b_value_set_boxed (value, &color);
      }
      break;
    case PROP_CELL_BACKGROUND_SET:
      b_value_set_boolean (value, cell->cell_background_set);
      break;
    case PROP_CELL_BACKGROUND:
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

}

static void
btk_cell_renderer_set_property (BObject      *object,
				buint         param_id,
				const BValue *value,
				BParamSpec   *pspec)
{
  BtkCellRenderer *cell = BTK_CELL_RENDERER (object);

  switch (param_id)
    {
    case PROP_MODE:
      cell->mode = b_value_get_enum (value);
      break;
    case PROP_VISIBLE:
      cell->visible = b_value_get_boolean (value);
      break;
    case PROP_SENSITIVE:
      cell->sensitive = b_value_get_boolean (value);
      break;
    case PROP_EDITING:
      cell->editing = b_value_get_boolean (value);
      break;
    case PROP_XALIGN:
      cell->xalign = b_value_get_float (value);
      break;
    case PROP_YALIGN:
      cell->yalign = b_value_get_float (value);
      break;
    case PROP_XPAD:
      cell->xpad = b_value_get_uint (value);
      break;
    case PROP_YPAD:
      cell->ypad = b_value_get_uint (value);
      break;
    case PROP_WIDTH:
      cell->width = b_value_get_int (value);
      break;
    case PROP_HEIGHT:
      cell->height = b_value_get_int (value);
      break;
    case PROP_IS_EXPANDER:
      cell->is_expander = b_value_get_boolean (value);
      break;
    case PROP_IS_EXPANDED:
      cell->is_expanded = b_value_get_boolean (value);
      break;
    case PROP_CELL_BACKGROUND:
      {
	BdkColor color;

	if (!b_value_get_string (value))
	  set_cell_bg_color (cell, NULL);
	else if (bdk_color_parse (b_value_get_string (value), &color))
	  set_cell_bg_color (cell, &color);
	else
	  g_warning ("Don't know color `%s'", b_value_get_string (value));

	g_object_notify (object, "cell-background-bdk");
      }
      break;
    case PROP_CELL_BACKGROUND_BDK:
      set_cell_bg_color (cell, b_value_get_boxed (value));
      break;
    case PROP_CELL_BACKGROUND_SET:
      cell->cell_background_set = b_value_get_boolean (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_cell_bg_color (BtkCellRenderer *cell,
		   BdkColor        *color)
{
  BtkCellRendererPrivate *priv = BTK_CELL_RENDERER_GET_PRIVATE (cell);

  if (color)
    {
      if (!cell->cell_background_set)
        {
	  cell->cell_background_set = TRUE;
	  g_object_notify (B_OBJECT (cell), "cell-background-set");
	}

      priv->cell_background.red = color->red;
      priv->cell_background.green = color->green;
      priv->cell_background.blue = color->blue;
    }
  else
    {
      if (cell->cell_background_set)
        {
	  cell->cell_background_set = FALSE;
	  g_object_notify (B_OBJECT (cell), "cell-background-set");
	}
    }
}

/**
 * btk_cell_renderer_get_size:
 * @cell: a #BtkCellRenderer
 * @widget: the widget the renderer is rendering to
 * @cell_area: (allow-none): The area a cell will be allocated, or %NULL
 * @x_offset: (out) (allow-none): location to return x offset of cell relative to @cell_area, or %NULL
 * @y_offset: (out) (allow-none): location to return y offset of cell relative to @cell_area, or %NULL
 * @width: (out) (allow-none): location to return width needed to render a cell, or %NULL
 * @height: (out) (allow-none): location to return height needed to render a cell, or %NULL
 *
 * Obtains the width and height needed to render the cell. Used by view 
 * widgets to determine the appropriate size for the cell_area passed to
 * btk_cell_renderer_render().  If @cell_area is not %NULL, fills in the
 * x and y offsets (if set) of the cell relative to this location. 
 *
 * Please note that the values set in @width and @height, as well as those 
 * in @x_offset and @y_offset are inclusive of the xpad and ypad properties.
 **/
void
btk_cell_renderer_get_size (BtkCellRenderer    *cell,
			    BtkWidget          *widget,
			    const BdkRectangle *cell_area,
			    bint               *x_offset,
			    bint               *y_offset,
			    bint               *width,
			    bint               *height)
{
  bint *real_width = width;
  bint *real_height = height;

  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (BTK_CELL_RENDERER_GET_CLASS (cell)->get_size != NULL);

  if (width && cell->width != -1)
    {
      real_width = NULL;
      *width = cell->width;
    }
  if (height && cell->height != -1)
    {
      real_height = NULL;
      *height = cell->height;
    }

  BTK_CELL_RENDERER_GET_CLASS (cell)->get_size (cell,
						widget,
						(BdkRectangle *) cell_area,
						x_offset,
						y_offset,
						real_width,
						real_height);
}

/**
 * btk_cell_renderer_render:
 * @cell: a #BtkCellRenderer
 * @window: a #BdkDrawable to draw to
 * @widget: the widget owning @window
 * @background_area: entire cell area (including tree expanders and maybe 
 *    padding on the sides)
 * @cell_area: area normally rendered by a cell renderer
 * @expose_area: area that actually needs updating
 * @flags: flags that affect rendering
 *
 * Invokes the virtual render function of the #BtkCellRenderer. The three
 * passed-in rectangles are areas of @window. Most renderers will draw within
 * @cell_area; the xalign, yalign, xpad, and ypad fields of the #BtkCellRenderer
 * should be honored with respect to @cell_area. @background_area includes the
 * blank space around the cell, and also the area containing the tree expander;
 * so the @background_area rectangles for all cells tile to cover the entire
 * @window.  @expose_area is a clip rectangle.
 **/
void
btk_cell_renderer_render (BtkCellRenderer      *cell,
			  BdkWindow            *window,
			  BtkWidget            *widget,
			  const BdkRectangle   *background_area,
			  const BdkRectangle   *cell_area,
			  const BdkRectangle   *expose_area,
			  BtkCellRendererState  flags)
{
  bboolean selected = FALSE;
  BtkCellRendererPrivate *priv = BTK_CELL_RENDERER_GET_PRIVATE (cell);

  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (BTK_CELL_RENDERER_GET_CLASS (cell)->render != NULL);

  selected = (flags & BTK_CELL_RENDERER_SELECTED) == BTK_CELL_RENDERER_SELECTED;

  if (cell->cell_background_set && !selected)
    {
      bairo_t *cr = bdk_bairo_create (window);

      bdk_bairo_rectangle (cr, background_area);
      bdk_bairo_set_source_color (cr, &priv->cell_background);
      bairo_fill (cr);
      
      bairo_destroy (cr);
    }

  BTK_CELL_RENDERER_GET_CLASS (cell)->render (cell,
					      window,
					      widget,
					      (BdkRectangle *) background_area,
					      (BdkRectangle *) cell_area,
					      (BdkRectangle *) expose_area,
					      flags);
}

/**
 * btk_cell_renderer_activate:
 * @cell: a #BtkCellRenderer
 * @event: a #BdkEvent
 * @widget: widget that received the event
 * @path: widget-dependent string representation of the event location; 
 *    e.g. for #BtkTreeView, a string representation of #BtkTreePath
 * @background_area: background area as passed to btk_cell_renderer_render()
 * @cell_area: cell area as passed to btk_cell_renderer_render()
 * @flags: render flags
 *
 * Passes an activate event to the cell renderer for possible processing.  
 * Some cell renderers may use events; for example, #BtkCellRendererToggle 
 * toggles when it gets a mouse click.
 *
 * Return value: %TRUE if the event was consumed/handled
 **/
bboolean
btk_cell_renderer_activate (BtkCellRenderer      *cell,
			    BdkEvent             *event,
			    BtkWidget            *widget,
			    const bchar          *path,
			    const BdkRectangle   *background_area,
			    const BdkRectangle   *cell_area,
			    BtkCellRendererState  flags)
{
  g_return_val_if_fail (BTK_IS_CELL_RENDERER (cell), FALSE);

  if (cell->mode != BTK_CELL_RENDERER_MODE_ACTIVATABLE)
    return FALSE;

  if (BTK_CELL_RENDERER_GET_CLASS (cell)->activate == NULL)
    return FALSE;

  return BTK_CELL_RENDERER_GET_CLASS (cell)->activate (cell,
						       event,
						       widget,
						       path,
						       (BdkRectangle *) background_area,
						       (BdkRectangle *) cell_area,
						       flags);
}

/**
 * btk_cell_renderer_start_editing:
 * @cell: a #BtkCellRenderer
 * @event: a #BdkEvent
 * @widget: widget that received the event
 * @path: widget-dependent string representation of the event location;
 *    e.g. for #BtkTreeView, a string representation of #BtkTreePath
 * @background_area: background area as passed to btk_cell_renderer_render()
 * @cell_area: cell area as passed to btk_cell_renderer_render()
 * @flags: render flags
 *
 * Passes an activate event to the cell renderer for possible processing.
 *
 * Return value: (transfer none): A new #BtkCellEditable, or %NULL
 **/
BtkCellEditable *
btk_cell_renderer_start_editing (BtkCellRenderer      *cell,
				 BdkEvent             *event,
				 BtkWidget            *widget,
				 const bchar          *path,
				 const BdkRectangle   *background_area,
				 const BdkRectangle   *cell_area,
				 BtkCellRendererState  flags)

{
  BtkCellEditable *editable;

  g_return_val_if_fail (BTK_IS_CELL_RENDERER (cell), NULL);

  if (cell->mode != BTK_CELL_RENDERER_MODE_EDITABLE)
    return NULL;

  if (BTK_CELL_RENDERER_GET_CLASS (cell)->start_editing == NULL)
    return NULL;

  editable = BTK_CELL_RENDERER_GET_CLASS (cell)->start_editing (cell,
								event,
								widget,
								path,
								(BdkRectangle *) background_area,
								(BdkRectangle *) cell_area,
								flags);

  g_signal_emit (cell, 
		 cell_renderer_signals[EDITING_STARTED], 0,
		 editable, path);

  cell->editing = TRUE;

  return editable;
}

/**
 * btk_cell_renderer_set_fixed_size:
 * @cell: A #BtkCellRenderer
 * @width: the width of the cell renderer, or -1
 * @height: the height of the cell renderer, or -1
 *
 * Sets the renderer size to be explicit, independent of the properties set.
 **/
void
btk_cell_renderer_set_fixed_size (BtkCellRenderer *cell,
				  bint             width,
				  bint             height)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (width >= -1 && height >= -1);

  if ((width != cell->width) || (height != cell->height))
    {
      g_object_freeze_notify (B_OBJECT (cell));

      if (width != cell->width)
        {
          cell->width = width;
          g_object_notify (B_OBJECT (cell), "width");
        }

      if (height != cell->height)
        {
          cell->height = height;
          g_object_notify (B_OBJECT (cell), "height");
        }

      g_object_thaw_notify (B_OBJECT (cell));
    }
}

/**
 * btk_cell_renderer_get_fixed_size:
 * @cell: A #BtkCellRenderer
 * @width: (out) (allow-none): location to fill in with the fixed width of the cell, or %NULL
 * @height: (out) (allow-none): location to fill in with the fixed height of the cell, or %NULL
 *
 * Fills in @width and @height with the appropriate size of @cell.
 **/
void
btk_cell_renderer_get_fixed_size (BtkCellRenderer *cell,
				  bint            *width,
				  bint            *height)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (width)
    *width = cell->width;
  if (height)
    *height = cell->height;
}

/**
 * btk_cell_renderer_set_alignment:
 * @cell: A #BtkCellRenderer
 * @xalign: the x alignment of the cell renderer
 * @yalign: the y alignment of the cell renderer
 *
 * Sets the renderer's alignment within its available space.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_set_alignment (BtkCellRenderer *cell,
                                 bfloat           xalign,
                                 bfloat           yalign)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (xalign >= 0.0 && xalign <= 1.0);
  g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

  if ((xalign != cell->xalign) || (yalign != cell->yalign))
    {
      g_object_freeze_notify (B_OBJECT (cell));

      if (xalign != cell->xalign)
        {
          cell->xalign = xalign;
          g_object_notify (B_OBJECT (cell), "xalign");
        }

      if (yalign != cell->yalign)
        {
          cell->yalign = yalign;
          g_object_notify (B_OBJECT (cell), "yalign");
        }

      g_object_thaw_notify (B_OBJECT (cell));
    }
}

/**
 * btk_cell_renderer_get_alignment:
 * @cell: A #BtkCellRenderer
 * @xalign: (out) (allow-none): location to fill in with the x alignment of the cell, or %NULL
 * @yalign: (out) (allow-none): location to fill in with the y alignment of the cell, or %NULL
 *
 * Fills in @xalign and @yalign with the appropriate values of @cell.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_get_alignment (BtkCellRenderer *cell,
                                 bfloat          *xalign,
                                 bfloat          *yalign)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (xalign)
    *xalign = cell->xalign;
  if (yalign)
    *yalign = cell->yalign;
}

/**
 * btk_cell_renderer_set_padding:
 * @cell: A #BtkCellRenderer
 * @xpad: the x padding of the cell renderer
 * @ypad: the y padding of the cell renderer
 *
 * Sets the renderer's padding.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_set_padding (BtkCellRenderer *cell,
                               bint             xpad,
                               bint             ypad)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));
  g_return_if_fail (xpad >= 0 && xpad >= 0);

  if ((xpad != cell->xpad) || (ypad != cell->ypad))
    {
      g_object_freeze_notify (B_OBJECT (cell));

      if (xpad != cell->xpad)
        {
          cell->xpad = xpad;
          g_object_notify (B_OBJECT (cell), "xpad");
        }

      if (ypad != cell->ypad)
        {
          cell->ypad = ypad;
          g_object_notify (B_OBJECT (cell), "ypad");
        }

      g_object_thaw_notify (B_OBJECT (cell));
    }
}

/**
 * btk_cell_renderer_get_padding:
 * @cell: A #BtkCellRenderer
 * @xpad: (out) (allow-none): location to fill in with the x padding of the cell, or %NULL
 * @ypad: (out) (allow-none): location to fill in with the y padding of the cell, or %NULL
 *
 * Fills in @xpad and @ypad with the appropriate values of @cell.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_get_padding (BtkCellRenderer *cell,
                               bint            *xpad,
                               bint            *ypad)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (xpad)
    *xpad = cell->xpad;
  if (ypad)
    *ypad = cell->ypad;
}

/**
 * btk_cell_renderer_set_visible:
 * @cell: A #BtkCellRenderer
 * @visible: the visibility of the cell
 *
 * Sets the cell renderer's visibility.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_set_visible (BtkCellRenderer *cell,
                               bboolean         visible)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (cell->visible != visible)
    {
      cell->visible = visible ? TRUE : FALSE;
      g_object_notify (B_OBJECT (cell), "visible");
    }
}

/**
 * btk_cell_renderer_get_visible:
 * @cell: A #BtkCellRenderer
 *
 * Returns the cell renderer's visibility.
 *
 * Returns: %TRUE if the cell renderer is visible
 *
 * Since: 2.18
 */
bboolean
btk_cell_renderer_get_visible (BtkCellRenderer *cell)
{
  g_return_val_if_fail (BTK_IS_CELL_RENDERER (cell), FALSE);

  return cell->visible;
}

/**
 * btk_cell_renderer_set_sensitive:
 * @cell: A #BtkCellRenderer
 * @sensitive: the sensitivity of the cell
 *
 * Sets the cell renderer's sensitivity.
 *
 * Since: 2.18
 **/
void
btk_cell_renderer_set_sensitive (BtkCellRenderer *cell,
                                 bboolean         sensitive)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (cell->sensitive != sensitive)
    {
      cell->sensitive = sensitive ? TRUE : FALSE;
      g_object_notify (B_OBJECT (cell), "sensitive");
    }
}

/**
 * btk_cell_renderer_get_sensitive:
 * @cell: A #BtkCellRenderer
 *
 * Returns the cell renderer's sensitivity.
 *
 * Returns: %TRUE if the cell renderer is sensitive
 *
 * Since: 2.18
 */
bboolean
btk_cell_renderer_get_sensitive (BtkCellRenderer *cell)
{
  g_return_val_if_fail (BTK_IS_CELL_RENDERER (cell), FALSE);

  return cell->sensitive;
}

/**
 * btk_cell_renderer_editing_canceled:
 * @cell: A #BtkCellRenderer
 * 
 * Causes the cell renderer to emit the #BtkCellRenderer::editing-canceled 
 * signal.  
 *
 * This function is for use only by implementations of cell renderers that 
 * need to notify the client program that an editing process was canceled 
 * and the changes were not committed.
 *
 * Since: 2.4
 * Deprecated: 2.6: Use btk_cell_renderer_stop_editing() instead
 **/
void
btk_cell_renderer_editing_canceled (BtkCellRenderer *cell)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  btk_cell_renderer_stop_editing (cell, TRUE);
}

/**
 * btk_cell_renderer_stop_editing:
 * @cell: A #BtkCellRenderer
 * @canceled: %TRUE if the editing has been canceled
 * 
 * Informs the cell renderer that the editing is stopped.
 * If @canceled is %TRUE, the cell renderer will emit the 
 * #BtkCellRenderer::editing-canceled signal. 
 *
 * This function should be called by cell renderer implementations 
 * in response to the #BtkCellEditable::editing-done signal of 
 * #BtkCellEditable.
 *
 * Since: 2.6
 **/
void
btk_cell_renderer_stop_editing (BtkCellRenderer *cell,
				bboolean         canceled)
{
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (cell->editing)
    {
      cell->editing = FALSE;
      if (canceled)
	g_signal_emit (cell, cell_renderer_signals[EDITING_CANCELED], 0);
    }
}

#define __BTK_CELL_RENDERER_C__
#include "btkaliasdef.c"
