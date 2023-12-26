/* btkellview.c
 * Copyright (C) 2002, 2003  Kristian Rietveld <kris@btk.org>
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
#include <string.h>
#include "btkcellview.h"
#include "btkcelllayout.h"
#include "btkintl.h"
#include "btkcellrenderertext.h"
#include "btkcellrendererpixbuf.h"
#include "btkprivate.h"
#include <bobject/gmarshal.h>
#include "btkbuildable.h"
#include "btkalias.h"

typedef struct _BtkCellViewCellInfo BtkCellViewCellInfo;
struct _BtkCellViewCellInfo
{
  BtkCellRenderer *cell;

  bint requested_width;
  bint real_width;
  buint expand : 1;
  buint pack : 1;

  GSList *attributes;

  BtkCellLayoutDataFunc func;
  bpointer func_data;
  GDestroyNotify destroy;
};

struct _BtkCellViewPrivate
{
  BtkTreeModel *model;
  BtkTreeRowReference *displayed_row;
  GList *cell_list;
  bint spacing;

  BdkColor background;
  bboolean background_set;
};


static void        btk_cell_view_cell_layout_init         (BtkCellLayoutIface *iface);
static void        btk_cell_view_get_property             (BObject           *object,
                                                           buint             param_id,
                                                           BValue           *value,
                                                           BParamSpec       *pspec);
static void        btk_cell_view_set_property             (BObject          *object,
                                                           buint             param_id,
                                                           const BValue     *value,
                                                           BParamSpec       *pspec);
static void        btk_cell_view_finalize                 (BObject          *object);
static void        btk_cell_view_size_request             (BtkWidget        *widget,
                                                           BtkRequisition   *requisition);
static void        btk_cell_view_size_allocate            (BtkWidget        *widget,
                                                           BtkAllocation    *allocation);
static bboolean    btk_cell_view_expose                   (BtkWidget        *widget,
                                                           BdkEventExpose   *event);
static void        btk_cell_view_set_value                (BtkCellView     *cell_view,
                                                           BtkCellRenderer *renderer,
                                                           bchar           *property,
                                                           BValue          *value);
static BtkCellViewCellInfo *btk_cell_view_get_cell_info   (BtkCellView      *cellview,
                                                           BtkCellRenderer  *renderer);
static void        btk_cell_view_set_cell_data            (BtkCellView      *cell_view);


static void        btk_cell_view_cell_layout_pack_start        (BtkCellLayout         *layout,
                                                                BtkCellRenderer       *renderer,
                                                                bboolean               expand);
static void        btk_cell_view_cell_layout_pack_end          (BtkCellLayout         *layout,
                                                                BtkCellRenderer       *renderer,
                                                                bboolean               expand);
static void        btk_cell_view_cell_layout_add_attribute     (BtkCellLayout         *layout,
                                                                BtkCellRenderer       *renderer,
                                                                const bchar           *attribute,
                                                                bint                   column);
static void       btk_cell_view_cell_layout_clear              (BtkCellLayout         *layout);
static void       btk_cell_view_cell_layout_clear_attributes   (BtkCellLayout         *layout,
                                                                BtkCellRenderer       *renderer);
static void       btk_cell_view_cell_layout_set_cell_data_func (BtkCellLayout         *layout,
                                                                BtkCellRenderer       *cell,
                                                                BtkCellLayoutDataFunc  func,
                                                                bpointer               func_data,
                                                                GDestroyNotify         destroy);
static void       btk_cell_view_cell_layout_reorder            (BtkCellLayout         *layout,
                                                                BtkCellRenderer       *cell,
                                                                bint                   position);
static GList *    btk_cell_view_cell_layout_get_cells          (BtkCellLayout         *layout);

/* buildable */
static void       btk_cell_view_buildable_init                 (BtkBuildableIface     *iface);
static bboolean   btk_cell_view_buildable_custom_tag_start     (BtkBuildable  	      *buildable,
								BtkBuilder    	      *builder,
								BObject       	      *child,
								const bchar   	      *tagname,
								GMarkupParser 	      *parser,
								bpointer      	      *data);
static void       btk_cell_view_buildable_custom_tag_end       (BtkBuildable  	      *buildable,
								BtkBuilder    	      *builder,
								BObject       	      *child,
								const bchar   	      *tagname,
								bpointer      	      *data);

static BtkBuildableIface *parent_buildable_iface;

#define BTK_CELL_VIEW_GET_PRIVATE(obj)    (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_CELL_VIEW, BtkCellViewPrivate))

enum
{
  PROP_0,
  PROP_BACKGROUND,
  PROP_BACKGROUND_BDK,
  PROP_BACKGROUND_SET,
  PROP_MODEL
};

G_DEFINE_TYPE_WITH_CODE (BtkCellView, btk_cell_view, BTK_TYPE_WIDGET, 
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_LAYOUT,
						btk_cell_view_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_cell_view_buildable_init))

static void
btk_cell_view_class_init (BtkCellViewClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);

  bobject_class->get_property = btk_cell_view_get_property;
  bobject_class->set_property = btk_cell_view_set_property;
  bobject_class->finalize = btk_cell_view_finalize;

  widget_class->expose_event = btk_cell_view_expose;
  widget_class->size_allocate = btk_cell_view_size_allocate;
  widget_class->size_request = btk_cell_view_size_request;

  /* properties */
  g_object_class_install_property (bobject_class,
                                   PROP_BACKGROUND,
                                   g_param_spec_string ("background",
                                                        P_("Background color name"),
                                                        P_("Background color as a string"),
                                                        NULL,
                                                        BTK_PARAM_WRITABLE));
  g_object_class_install_property (bobject_class,
                                   PROP_BACKGROUND_BDK,
                                   g_param_spec_boxed ("background-bdk",
                                                      P_("Background color"),
                                                      P_("Background color as a BdkColor"),
                                                      BDK_TYPE_COLOR,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkCellView:model
   *
   * The model for cell view
   *
   * since 2.10
   */
  g_object_class_install_property (bobject_class,
				   PROP_MODEL,
				   g_param_spec_object  ("model",
							 P_("CellView model"),
							 P_("The model for cell view"),
							 BTK_TYPE_TREE_MODEL,
							 BTK_PARAM_READWRITE));
  
#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (bobject_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, BTK_PARAM_READWRITE))

  ADD_SET_PROP ("background-set", PROP_BACKGROUND_SET,
                P_("Background set"),
                P_("Whether this tag affects the background color"));

  g_type_class_add_private (bobject_class, sizeof (BtkCellViewPrivate));
}

static void
btk_cell_view_buildable_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = _btk_cell_layout_buildable_add_child;
  iface->custom_tag_start = btk_cell_view_buildable_custom_tag_start;
  iface->custom_tag_end = btk_cell_view_buildable_custom_tag_end;
}

static void
btk_cell_view_cell_layout_init (BtkCellLayoutIface *iface)
{
  iface->pack_start = btk_cell_view_cell_layout_pack_start;
  iface->pack_end = btk_cell_view_cell_layout_pack_end;
  iface->clear = btk_cell_view_cell_layout_clear;
  iface->add_attribute = btk_cell_view_cell_layout_add_attribute;
  iface->set_cell_data_func = btk_cell_view_cell_layout_set_cell_data_func;
  iface->clear_attributes = btk_cell_view_cell_layout_clear_attributes;
  iface->reorder = btk_cell_view_cell_layout_reorder;
  iface->get_cells = btk_cell_view_cell_layout_get_cells;
}

static void
btk_cell_view_get_property (BObject    *object,
                            buint       param_id,
                            BValue     *value,
                            BParamSpec *pspec)
{
  BtkCellView *view = BTK_CELL_VIEW (object);

  switch (param_id)
    {
      case PROP_BACKGROUND_BDK:
        {
          BdkColor color;

          color = view->priv->background;

          b_value_set_boxed (value, &color);
        }
        break;
      case PROP_BACKGROUND_SET:
        b_value_set_boolean (value, view->priv->background_set);
        break;
      case PROP_MODEL:
	b_value_set_object (value, view->priv->model);
	break;
      default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
btk_cell_view_set_property (BObject      *object,
                            buint         param_id,
                            const BValue *value,
                            BParamSpec   *pspec)
{
  BtkCellView *view = BTK_CELL_VIEW (object);

  switch (param_id)
    {
      case PROP_BACKGROUND:
        {
          BdkColor color;

          if (!b_value_get_string (value))
            btk_cell_view_set_background_color (view, NULL);
          else if (bdk_color_parse (b_value_get_string (value), &color))
            btk_cell_view_set_background_color (view, &color);
          else
            g_warning ("Don't know color `%s'", b_value_get_string (value));

          g_object_notify (object, "background-bdk");
        }
        break;
      case PROP_BACKGROUND_BDK:
        btk_cell_view_set_background_color (view, b_value_get_boxed (value));
        break;
      case PROP_BACKGROUND_SET:
        view->priv->background_set = b_value_get_boolean (value);
        break;
      case PROP_MODEL:
	btk_cell_view_set_model (view, b_value_get_object (value));
	break;
    default:
        B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
btk_cell_view_init (BtkCellView *cellview)
{
  btk_widget_set_has_window (BTK_WIDGET (cellview), FALSE);

  cellview->priv = BTK_CELL_VIEW_GET_PRIVATE (cellview);
}

static void
btk_cell_view_finalize (BObject *object)
{
  BtkCellView *cellview = BTK_CELL_VIEW (object);

  btk_cell_view_cell_layout_clear (BTK_CELL_LAYOUT (cellview));

  if (cellview->priv->model)
     g_object_unref (cellview->priv->model);

  if (cellview->priv->displayed_row)
     btk_tree_row_reference_free (cellview->priv->displayed_row);

  B_OBJECT_CLASS (btk_cell_view_parent_class)->finalize (object);
}

static void
btk_cell_view_size_request (BtkWidget      *widget,
                            BtkRequisition *requisition)
{
  GList *i;
  bboolean first_cell = TRUE;
  BtkCellView *cellview;

  cellview = BTK_CELL_VIEW (widget);

  requisition->width = 0;
  requisition->height = 0;

  if (cellview->priv->displayed_row)
    btk_cell_view_set_cell_data (cellview);

  for (i = cellview->priv->cell_list; i; i = i->next)
    {
      bint width, height;
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)i->data;

      if (!info->cell->visible)
        continue;

      if (!first_cell)
        requisition->width += cellview->priv->spacing;

      btk_cell_renderer_get_size (info->cell, widget, NULL, NULL, NULL,
                                  &width, &height);

      info->requested_width = width;
      requisition->width += width;
      requisition->height = MAX (requisition->height, height);

      first_cell = FALSE;
    }
}

static void
btk_cell_view_size_allocate (BtkWidget     *widget,
                             BtkAllocation *allocation)
{
  GList *i;
  bint expand_cell_count = 0;
  bint full_requested_width = 0;
  bint extra_space;
  BtkCellView *cellview;

  widget->allocation = *allocation;

  cellview = BTK_CELL_VIEW (widget);

  /* checking how much extra space we have */
  for (i = cellview->priv->cell_list; i; i = i->next)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)i->data;

      if (!info->cell->visible)
        continue;

      if (info->expand)
        expand_cell_count++;

      full_requested_width += info->requested_width;
    }

  extra_space = widget->allocation.width - full_requested_width;
  if (extra_space < 0)
    extra_space = 0;
  else if (extra_space > 0 && expand_cell_count > 0)
    extra_space /= expand_cell_count;

  for (i = cellview->priv->cell_list; i; i = i->next)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)i->data;

      if (!info->cell->visible)
        continue;

      info->real_width = info->requested_width +
        (info->expand ? extra_space : 0);
    }
}

static bboolean
btk_cell_view_expose (BtkWidget      *widget,
                      BdkEventExpose *event)
{
  GList *i;
  BtkCellView *cellview;
  BdkRectangle area;
  BtkCellRendererState state;
  bboolean rtl = (btk_widget_get_direction(widget) == BTK_TEXT_DIR_RTL);

  cellview = BTK_CELL_VIEW (widget);

  if (!btk_widget_is_drawable (widget))
    return FALSE;

  /* "blank" background */
  if (cellview->priv->background_set)
    {
      bairo_t *cr = bdk_bairo_create (BTK_WIDGET (cellview)->window);

      bdk_bairo_rectangle (cr, &widget->allocation);
      bairo_set_source_rgb (cr,
			    cellview->priv->background.red / 65535.,
			    cellview->priv->background.green / 65535.,
			    cellview->priv->background.blue / 65535.);
      bairo_fill (cr);

      bairo_destroy (cr);
    }

  /* set cell data (if available) */
  if (cellview->priv->displayed_row)
    btk_cell_view_set_cell_data (cellview);
  else if (cellview->priv->model)
    return FALSE;

  /* render cells */
  area = widget->allocation;

  /* we draw on our very own window, initialize x and y to zero */
  area.x = widget->allocation.x + (rtl ? widget->allocation.width : 0); 
  area.y = widget->allocation.y;

  if (btk_widget_get_state (widget) == BTK_STATE_PRELIGHT)
    state = BTK_CELL_RENDERER_PRELIT;
  else if (btk_widget_get_state (widget) == BTK_STATE_INSENSITIVE)
    state = BTK_CELL_RENDERER_INSENSITIVE;
  else
    state = 0;
      
  /* PACK_START */
  for (i = cellview->priv->cell_list; i; i = i->next)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)i->data;

      if (info->pack == BTK_PACK_END)
        continue;

      if (!info->cell->visible)
        continue;

      area.width = info->real_width;
      if (rtl)                                             
         area.x -= area.width;

      btk_cell_renderer_render (info->cell,
                                event->window,
                                widget,
                                /* FIXME! */
                                &area, &area, &event->area, state);

      if (!rtl)                                           
         area.x += info->real_width;
    }

   area.x = rtl ? widget->allocation.x : (widget->allocation.x + widget->allocation.width);  

  /* PACK_END */
  for (i = cellview->priv->cell_list; i; i = i->next)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)i->data;

      if (info->pack == BTK_PACK_START)
        continue;

      if (!info->cell->visible)
        continue;

      area.width = info->real_width;
      if (!rtl)
         area.x -= area.width;   

      btk_cell_renderer_render (info->cell,
                                widget->window,
                                widget,
                                /* FIXME ! */
                                &area, &area, &event->area, state);
      if (rtl)
         area.x += info->real_width;
    }

  return FALSE;
}

static BtkCellViewCellInfo *
btk_cell_view_get_cell_info (BtkCellView     *cellview,
                             BtkCellRenderer *renderer)
{
  GList *i;

  for (i = cellview->priv->cell_list; i; i = i->next)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)i->data;

      if (info->cell == renderer)
        return info;
    }

  return NULL;
}

static void
btk_cell_view_set_cell_data (BtkCellView *cell_view)
{
  GList *i;
  BtkTreeIter iter;
  BtkTreePath *path;

  g_return_if_fail (cell_view->priv->displayed_row != NULL);

  path = btk_tree_row_reference_get_path (cell_view->priv->displayed_row);
  if (!path)
    return;

  btk_tree_model_get_iter (cell_view->priv->model, &iter, path);
  btk_tree_path_free (path);

  for (i = cell_view->priv->cell_list; i; i = i->next)
    {
      GSList *j;
      BtkCellViewCellInfo *info = i->data;

      g_object_freeze_notify (B_OBJECT (info->cell));

      for (j = info->attributes; j && j->next; j = j->next->next)
        {
          bchar *property = j->data;
          bint column = BPOINTER_TO_INT (j->next->data);
          BValue value = {0, };

          btk_tree_model_get_value (cell_view->priv->model, &iter,
                                    column, &value);
          g_object_set_property (B_OBJECT (info->cell),
                                 property, &value);
          b_value_unset (&value);
        }

      if (info->func)
	(* info->func) (BTK_CELL_LAYOUT (cell_view),
			info->cell,
			cell_view->priv->model,
			&iter,
			info->func_data);

      g_object_thaw_notify (B_OBJECT (info->cell));
    }
}

/* BtkCellLayout implementation */
static void
btk_cell_view_cell_layout_pack_start (BtkCellLayout   *layout,
                                      BtkCellRenderer *renderer,
                                      bboolean         expand)
{
  BtkCellViewCellInfo *info;
  BtkCellView *cellview = BTK_CELL_VIEW (layout);

  g_return_if_fail (!btk_cell_view_get_cell_info (cellview, renderer));

  g_object_ref_sink (renderer);

  info = g_slice_new0 (BtkCellViewCellInfo);
  info->cell = renderer;
  info->expand = expand ? TRUE : FALSE;
  info->pack = BTK_PACK_START;

  cellview->priv->cell_list = g_list_append (cellview->priv->cell_list, info);

  btk_widget_queue_resize (BTK_WIDGET (cellview));
}

static void
btk_cell_view_cell_layout_pack_end (BtkCellLayout   *layout,
                                    BtkCellRenderer *renderer,
                                    bboolean         expand)
{
  BtkCellViewCellInfo *info;
  BtkCellView *cellview = BTK_CELL_VIEW (layout);

  g_return_if_fail (!btk_cell_view_get_cell_info (cellview, renderer));

  g_object_ref_sink (renderer);

  info = g_slice_new0 (BtkCellViewCellInfo);
  info->cell = renderer;
  info->expand = expand ? TRUE : FALSE;
  info->pack = BTK_PACK_END;

  cellview->priv->cell_list = g_list_append (cellview->priv->cell_list, info);

  btk_widget_queue_resize (BTK_WIDGET (cellview));
}

static void
btk_cell_view_cell_layout_add_attribute (BtkCellLayout   *layout,
                                         BtkCellRenderer *renderer,
                                         const bchar     *attribute,
                                         bint             column)
{
  BtkCellViewCellInfo *info;
  BtkCellView *cellview = BTK_CELL_VIEW (layout);

  info = btk_cell_view_get_cell_info (cellview, renderer);
  g_return_if_fail (info != NULL);

  info->attributes = b_slist_prepend (info->attributes,
                                      BINT_TO_POINTER (column));
  info->attributes = b_slist_prepend (info->attributes,
                                      g_strdup (attribute));
}

static void
btk_cell_view_cell_layout_clear (BtkCellLayout *layout)
{
  BtkCellView *cellview = BTK_CELL_VIEW (layout);

  while (cellview->priv->cell_list)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)cellview->priv->cell_list->data;

      btk_cell_view_cell_layout_clear_attributes (layout, info->cell);
      g_object_unref (info->cell);
      g_slice_free (BtkCellViewCellInfo, info);
      cellview->priv->cell_list = g_list_delete_link (cellview->priv->cell_list, 
						      cellview->priv->cell_list);
    }

  btk_widget_queue_resize (BTK_WIDGET (cellview));
}

static void
btk_cell_view_cell_layout_set_cell_data_func (BtkCellLayout         *layout,
                                              BtkCellRenderer       *cell,
                                              BtkCellLayoutDataFunc  func,
                                              bpointer               func_data,
                                              GDestroyNotify         destroy)
{
  BtkCellView *cellview = BTK_CELL_VIEW (layout);
  BtkCellViewCellInfo *info;

  info = btk_cell_view_get_cell_info (cellview, cell);
  g_return_if_fail (info != NULL);

  if (info->destroy)
    {
      GDestroyNotify d = info->destroy;

      info->destroy = NULL;
      d (info->func_data);
    }

  info->func = func;
  info->func_data = func_data;
  info->destroy = destroy;
}

static void
btk_cell_view_cell_layout_clear_attributes (BtkCellLayout   *layout,
                                            BtkCellRenderer *renderer)
{
  BtkCellView *cellview = BTK_CELL_VIEW (layout);
  BtkCellViewCellInfo *info;
  GSList *list;

  info = btk_cell_view_get_cell_info (cellview, renderer);
  if (info != NULL)
    {
      list = info->attributes;
      while (list && list->next)
	{
	  g_free (list->data);
	  list = list->next->next;
	}
      
      b_slist_free (info->attributes);
      info->attributes = NULL;
    }
}

static void
btk_cell_view_cell_layout_reorder (BtkCellLayout   *layout,
                                   BtkCellRenderer *cell,
                                   bint             position)
{
  BtkCellView *cellview = BTK_CELL_VIEW (layout);
  BtkCellViewCellInfo *info;
  GList *link;

  info = btk_cell_view_get_cell_info (cellview, cell);

  g_return_if_fail (info != NULL);
  g_return_if_fail (position >= 0);

  link = g_list_find (cellview->priv->cell_list, info);

  g_return_if_fail (link != NULL);

  cellview->priv->cell_list = g_list_delete_link (cellview->priv->cell_list,
                                                  link);
  cellview->priv->cell_list = g_list_insert (cellview->priv->cell_list,
                                             info, position);

  btk_widget_queue_draw (BTK_WIDGET (cellview));
}

/**
 * btk_cell_view_new:
 *
 * Creates a new #BtkCellView widget.
 *
 * Return value: A newly created #BtkCellView widget.
 *
 * Since: 2.6
 */
BtkWidget *
btk_cell_view_new (void)
{
  BtkCellView *cellview;

  cellview = g_object_new (btk_cell_view_get_type (), NULL);

  return BTK_WIDGET (cellview);
}

/**
 * btk_cell_view_new_with_text:
 * @text: the text to display in the cell view
 *
 * Creates a new #BtkCellView widget, adds a #BtkCellRendererText 
 * to it, and makes its show @text.
 *
 * Return value: A newly created #BtkCellView widget.
 *
 * Since: 2.6
 */
BtkWidget *
btk_cell_view_new_with_text (const bchar *text)
{
  BtkCellView *cellview;
  BtkCellRenderer *renderer;
  BValue value = {0, };

  cellview = BTK_CELL_VIEW (btk_cell_view_new ());

  renderer = btk_cell_renderer_text_new ();
  btk_cell_view_cell_layout_pack_start (BTK_CELL_LAYOUT (cellview),
                                        renderer, TRUE);

  b_value_init (&value, B_TYPE_STRING);
  b_value_set_string (&value, text);
  btk_cell_view_set_value (cellview, renderer, "text", &value);
  b_value_unset (&value);

  return BTK_WIDGET (cellview);
}

/**
 * btk_cell_view_new_with_markup:
 * @markup: the text to display in the cell view
 *
 * Creates a new #BtkCellView widget, adds a #BtkCellRendererText 
 * to it, and makes it show @markup. The text can be
 * marked up with the <link linkend="BangoMarkupFormat">Bango text 
 * markup language</link>.
 *
 * Return value: A newly created #BtkCellView widget.
 *
 * Since: 2.6
 */
BtkWidget *
btk_cell_view_new_with_markup (const bchar *markup)
{
  BtkCellView *cellview;
  BtkCellRenderer *renderer;
  BValue value = {0, };

  cellview = BTK_CELL_VIEW (btk_cell_view_new ());

  renderer = btk_cell_renderer_text_new ();
  btk_cell_view_cell_layout_pack_start (BTK_CELL_LAYOUT (cellview),
                                        renderer, TRUE);

  b_value_init (&value, B_TYPE_STRING);
  b_value_set_string (&value, markup);
  btk_cell_view_set_value (cellview, renderer, "markup", &value);
  b_value_unset (&value);

  return BTK_WIDGET (cellview);
}

/**
 * btk_cell_view_new_with_pixbuf:
 * @pixbuf: the image to display in the cell view
 *
 * Creates a new #BtkCellView widget, adds a #BtkCellRendererPixbuf 
 * to it, and makes its show @pixbuf. 
 *
 * Return value: A newly created #BtkCellView widget.
 *
 * Since: 2.6
 */
BtkWidget *
btk_cell_view_new_with_pixbuf (BdkPixbuf *pixbuf)
{
  BtkCellView *cellview;
  BtkCellRenderer *renderer;
  BValue value = {0, };

  cellview = BTK_CELL_VIEW (btk_cell_view_new ());

  renderer = btk_cell_renderer_pixbuf_new ();
  btk_cell_view_cell_layout_pack_start (BTK_CELL_LAYOUT (cellview),
                                        renderer, TRUE);

  b_value_init (&value, BDK_TYPE_PIXBUF);
  b_value_set_object (&value, pixbuf);
  btk_cell_view_set_value (cellview, renderer, "pixbuf", &value);
  b_value_unset (&value);

  return BTK_WIDGET (cellview);
}

/**
 * btk_cell_view_set_value:
 * @cell_view: a #BtkCellView widget
 * @renderer: one of the renderers of @cell_view
 * @property: the name of the property of @renderer to set
 * @value: the new value to set the property to
 * 
 * Sets a property of a cell renderer of @cell_view, and
 * makes sure the display of @cell_view is updated.
 *
 * Since: 2.6
 */
static void
btk_cell_view_set_value (BtkCellView     *cell_view,
                         BtkCellRenderer *renderer,
                         bchar           *property,
                         BValue          *value)
{
  g_object_set_property (B_OBJECT (renderer), property, value);

  /* force resize and redraw */
  btk_widget_queue_resize (BTK_WIDGET (cell_view));
}

/**
 * btk_cell_view_set_model:
 * @cell_view: a #BtkCellView
 * @model: (allow-none): a #BtkTreeModel
 *
 * Sets the model for @cell_view.  If @cell_view already has a model
 * set, it will remove it before setting the new model.  If @model is
 * %NULL, then it will unset the old model.
 *
 * Since: 2.6
 */
void
btk_cell_view_set_model (BtkCellView  *cell_view,
                         BtkTreeModel *model)
{
  g_return_if_fail (BTK_IS_CELL_VIEW (cell_view));
  g_return_if_fail (model == NULL || BTK_IS_TREE_MODEL (model));

  if (cell_view->priv->model)
    {
      if (cell_view->priv->displayed_row)
        btk_tree_row_reference_free (cell_view->priv->displayed_row);
      cell_view->priv->displayed_row = NULL;

      g_object_unref (cell_view->priv->model);
      cell_view->priv->model = NULL;
    }

  cell_view->priv->model = model;

  if (cell_view->priv->model)
    g_object_ref (cell_view->priv->model);

  btk_widget_queue_resize (BTK_WIDGET (cell_view));
}

/**
 * btk_cell_view_get_model:
 * @cell_view: a #BtkCellView
 *
 * Returns the model for @cell_view. If no model is used %NULL is
 * returned.
 *
 * Returns: (transfer none): a #BtkTreeModel used or %NULL
 *
 * Since: 2.16
 **/
BtkTreeModel *
btk_cell_view_get_model (BtkCellView *cell_view)
{
  g_return_val_if_fail (BTK_IS_CELL_VIEW (cell_view), NULL);

  return cell_view->priv->model;
}

/**
 * btk_cell_view_set_displayed_row:
 * @cell_view: a #BtkCellView
 * @path: (allow-none): a #BtkTreePath or %NULL to unset.
 *
 * Sets the row of the model that is currently displayed
 * by the #BtkCellView. If the path is unset, then the
 * contents of the cellview "stick" at their last value;
 * this is not normally a desired result, but may be
 * a needed intermediate state if say, the model for
 * the #BtkCellView becomes temporarily empty.
 *
 * Since: 2.6
 **/
void
btk_cell_view_set_displayed_row (BtkCellView *cell_view,
                                 BtkTreePath *path)
{
  g_return_if_fail (BTK_IS_CELL_VIEW (cell_view));
  g_return_if_fail (BTK_IS_TREE_MODEL (cell_view->priv->model));

  if (cell_view->priv->displayed_row)
    btk_tree_row_reference_free (cell_view->priv->displayed_row);

  if (path)
    {
      cell_view->priv->displayed_row =
	btk_tree_row_reference_new (cell_view->priv->model, path);
    }
  else
    cell_view->priv->displayed_row = NULL;

  /* force resize and redraw */
  btk_widget_queue_resize (BTK_WIDGET (cell_view));
}

/**
 * btk_cell_view_get_displayed_row:
 * @cell_view: a #BtkCellView
 *
 * Returns a #BtkTreePath referring to the currently 
 * displayed row. If no row is currently displayed, 
 * %NULL is returned.
 *
 * Returns: the currently displayed row or %NULL
 *
 * Since: 2.6
 */
BtkTreePath *
btk_cell_view_get_displayed_row (BtkCellView *cell_view)
{
  g_return_val_if_fail (BTK_IS_CELL_VIEW (cell_view), NULL);

  if (!cell_view->priv->displayed_row)
    return NULL;

  return btk_tree_row_reference_get_path (cell_view->priv->displayed_row);
}

/**
 * btk_cell_view_get_size_of_row:
 * @cell_view: a #BtkCellView
 * @path: a #BtkTreePath 
 * @requisition: (out): return location for the size 
 *
 * Sets @requisition to the size needed by @cell_view to display 
 * the model row pointed to by @path.
 * 
 * Return value: %TRUE
 *
 * Since: 2.6
 */
bboolean
btk_cell_view_get_size_of_row (BtkCellView    *cell_view,
                               BtkTreePath    *path,
                               BtkRequisition *requisition)
{
  BtkTreeRowReference *tmp;
  BtkRequisition req;

  g_return_val_if_fail (BTK_IS_CELL_VIEW (cell_view), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (requisition != NULL, FALSE);

  tmp = cell_view->priv->displayed_row;
  cell_view->priv->displayed_row =
    btk_tree_row_reference_new (cell_view->priv->model, path);

  btk_cell_view_size_request (BTK_WIDGET (cell_view), requisition);

  btk_tree_row_reference_free (cell_view->priv->displayed_row);
  cell_view->priv->displayed_row = tmp;

  /* restore actual size info */
  btk_cell_view_size_request (BTK_WIDGET (cell_view), &req);

  return TRUE;
}

/**
 * btk_cell_view_set_background_color:
 * @cell_view: a #BtkCellView
 * @color: the new background color
 *
 * Sets the background color of @view.
 *
 * Since: 2.6
 */
void
btk_cell_view_set_background_color (BtkCellView    *cell_view,
                                    const BdkColor *color)
{
  g_return_if_fail (BTK_IS_CELL_VIEW (cell_view));

  if (color)
    {
      if (!cell_view->priv->background_set)
        {
          cell_view->priv->background_set = TRUE;
          g_object_notify (B_OBJECT (cell_view), "background-set");
        }

      cell_view->priv->background = *color;
    }
  else
    {
      if (cell_view->priv->background_set)
        {
          cell_view->priv->background_set = FALSE;
          g_object_notify (B_OBJECT (cell_view), "background-set");
        }
    }

  btk_widget_queue_draw (BTK_WIDGET (cell_view));
}

static GList *
btk_cell_view_cell_layout_get_cells (BtkCellLayout *layout)
{
  BtkCellView *cell_view = BTK_CELL_VIEW (layout);
  GList *retval = NULL, *list;

  g_return_val_if_fail (cell_view != NULL, NULL);

  btk_cell_view_set_cell_data (cell_view);

  for (list = cell_view->priv->cell_list; list; list = list->next)
    {
      BtkCellViewCellInfo *info = (BtkCellViewCellInfo *)list->data;

      retval = g_list_prepend (retval, info->cell);
    }

  return g_list_reverse (retval);
}

/**
 * btk_cell_view_get_cell_renderers:
 * @cell_view: a #BtkCellView
 *
 * Returns the cell renderers which have been added to @cell_view.
 *
 * Return value: a list of cell renderers. The list, but not the
 *   renderers has been newly allocated and should be freed with
 *   g_list_free() when no longer needed.
 *
 * Since: 2.6
 *
 * Deprecated: 2.18: use btk_cell_layout_get_cells() instead.
 **/
GList *
btk_cell_view_get_cell_renderers (BtkCellView *cell_view)
{
  return btk_cell_view_cell_layout_get_cells (BTK_CELL_LAYOUT (cell_view));
}

static bboolean
btk_cell_view_buildable_custom_tag_start (BtkBuildable  *buildable,
					  BtkBuilder    *builder,
					  BObject       *child,
					  const bchar   *tagname,
					  GMarkupParser *parser,
					  bpointer      *data)
{
  if (parent_buildable_iface->custom_tag_start &&
      parent_buildable_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, data))
    return TRUE;

  return _btk_cell_layout_buildable_custom_tag_start (buildable, builder, child,
						      tagname, parser, data);
}

static void
btk_cell_view_buildable_custom_tag_end (BtkBuildable *buildable,
					BtkBuilder   *builder,
					BObject      *child,
					const bchar  *tagname,
					bpointer     *data)
{
  if (strcmp (tagname, "attributes") == 0)
    _btk_cell_layout_buildable_custom_tag_end (buildable, builder, child, tagname,
					       data);
  else if (parent_buildable_iface->custom_tag_end)
    parent_buildable_iface->custom_tag_end (buildable, builder, child, tagname,
					    data);
}


#define __BTK_CELL_VIEW_C__
#include "btkaliasdef.c"
