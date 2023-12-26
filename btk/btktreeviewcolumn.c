/* btktreeviewcolumn.c
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include "btktreeviewcolumn.h"
#include "btktreeview.h"
#include "btktreeprivate.h"
#include "btkcelllayout.h"
#include "btkbutton.h"
#include "btkalignment.h"
#include "btklabel.h"
#include "btkhbox.h"
#include "btkmarshalers.h"
#include "btkarrow.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

enum
{
  PROP_0,
  PROP_VISIBLE,
  PROP_RESIZABLE,
  PROP_WIDTH,
  PROP_SPACING,
  PROP_SIZING,
  PROP_FIXED_WIDTH,
  PROP_MIN_WIDTH,
  PROP_MAX_WIDTH,
  PROP_TITLE,
  PROP_EXPAND,
  PROP_CLICKABLE,
  PROP_WIDGET,
  PROP_ALIGNMENT,
  PROP_REORDERABLE,
  PROP_SORT_INDICATOR,
  PROP_SORT_ORDER,
  PROP_SORT_COLUMN_ID
};

enum
{
  CLICKED,
  LAST_SIGNAL
};

typedef struct _BtkTreeViewColumnCellInfo BtkTreeViewColumnCellInfo;
struct _BtkTreeViewColumnCellInfo
{
  BtkCellRenderer *cell;
  GSList *attributes;
  BtkTreeCellDataFunc func;
  gpointer func_data;
  GDestroyNotify destroy;
  gint requested_width;
  gint real_width;
  guint expand : 1;
  guint pack : 1;
  guint has_focus : 1;
  guint in_editing_mode : 1;
};

/* Type methods */
static void btk_tree_view_column_cell_layout_init              (BtkCellLayoutIface      *iface);

/* GObject methods */
static void btk_tree_view_column_set_property                  (GObject                 *object,
								guint                    prop_id,
								const GValue            *value,
								GParamSpec              *pspec);
static void btk_tree_view_column_get_property                  (GObject                 *object,
								guint                    prop_id,
								GValue                  *value,
								GParamSpec              *pspec);
static void btk_tree_view_column_finalize                      (GObject                 *object);

/* BtkCellLayout implementation */
static void btk_tree_view_column_cell_layout_pack_start         (BtkCellLayout         *cell_layout,
                                                                 BtkCellRenderer       *cell,
                                                                 gboolean               expand);
static void btk_tree_view_column_cell_layout_pack_end           (BtkCellLayout         *cell_layout,
                                                                 BtkCellRenderer       *cell,
                                                                 gboolean               expand);
static void btk_tree_view_column_cell_layout_clear              (BtkCellLayout         *cell_layout);
static void btk_tree_view_column_cell_layout_add_attribute      (BtkCellLayout         *cell_layout,
                                                                 BtkCellRenderer       *cell,
                                                                 const gchar           *attribute,
                                                                 gint                   column);
static void btk_tree_view_column_cell_layout_set_cell_data_func (BtkCellLayout         *cell_layout,
                                                                 BtkCellRenderer       *cell,
                                                                 BtkCellLayoutDataFunc  func,
                                                                 gpointer               func_data,
                                                                 GDestroyNotify         destroy);
static void btk_tree_view_column_cell_layout_clear_attributes   (BtkCellLayout         *cell_layout,
                                                                 BtkCellRenderer       *cell);
static void btk_tree_view_column_cell_layout_reorder            (BtkCellLayout         *cell_layout,
                                                                 BtkCellRenderer       *cell,
                                                                 gint                   position);
static GList *btk_tree_view_column_cell_layout_get_cells        (BtkCellLayout         *cell_layout);

/* Button handling code */
static void btk_tree_view_column_create_button                 (BtkTreeViewColumn       *tree_column);
static void btk_tree_view_column_update_button                 (BtkTreeViewColumn       *tree_column);

/* Button signal handlers */
static gint btk_tree_view_column_button_event                  (BtkWidget               *widget,
								BdkEvent                *event,
								gpointer                 data);
static void btk_tree_view_column_button_clicked                (BtkWidget               *widget,
								gpointer                 data);
static gboolean btk_tree_view_column_mnemonic_activate         (BtkWidget *widget,
					                        gboolean   group_cycling,
								gpointer   data);

/* Property handlers */
static void btk_tree_view_model_sort_column_changed            (BtkTreeSortable         *sortable,
								BtkTreeViewColumn       *tree_column);

/* Internal functions */
static void btk_tree_view_column_sort                          (BtkTreeViewColumn       *tree_column,
								gpointer                 data);
static void btk_tree_view_column_setup_sort_column_id_callback (BtkTreeViewColumn       *tree_column);
static void btk_tree_view_column_set_attributesv               (BtkTreeViewColumn       *tree_column,
								BtkCellRenderer         *cell_renderer,
								va_list                  args);
static BtkTreeViewColumnCellInfo *btk_tree_view_column_get_cell_info (BtkTreeViewColumn *tree_column,
								      BtkCellRenderer   *cell_renderer);

/* cell list manipulation */
static GList *btk_tree_view_column_cell_first                  (BtkTreeViewColumn      *tree_column);
static GList *btk_tree_view_column_cell_last                   (BtkTreeViewColumn      *tree_column);
static GList *btk_tree_view_column_cell_next                   (BtkTreeViewColumn      *tree_column,
								GList                  *current);
static GList *btk_tree_view_column_cell_prev                   (BtkTreeViewColumn      *tree_column,
								GList                  *current);
static void btk_tree_view_column_clear_attributes_by_info      (BtkTreeViewColumn      *tree_column,
					                        BtkTreeViewColumnCellInfo *info);
/* BtkBuildable implementation */
static void btk_tree_view_column_buildable_init                 (BtkBuildableIface     *iface);

static guint tree_column_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BtkTreeViewColumn, btk_tree_view_column, BTK_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_LAYOUT,
						btk_tree_view_column_cell_layout_init)
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_tree_view_column_buildable_init))


static void
btk_tree_view_column_class_init (BtkTreeViewColumnClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  class->clicked = NULL;

  object_class->finalize = btk_tree_view_column_finalize;
  object_class->set_property = btk_tree_view_column_set_property;
  object_class->get_property = btk_tree_view_column_get_property;
  
  tree_column_signals[CLICKED] =
    g_signal_new (I_("clicked"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkTreeViewColumnClass, clicked),
                  NULL, NULL,
                  _btk_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   PROP_VISIBLE,
                                   g_param_spec_boolean ("visible",
                                                        P_("Visible"),
                                                        P_("Whether to display the column"),
                                                         TRUE,
                                                         BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_RESIZABLE,
                                   g_param_spec_boolean ("resizable",
							 P_("Resizable"),
							 P_("Column is user-resizable"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_WIDTH,
                                   g_param_spec_int ("width",
						     P_("Width"),
						     P_("Current width of the column"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READABLE));
  g_object_class_install_property (object_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
						     P_("Spacing"),
						     P_("Space which is inserted between cells"),
						     0,
						     G_MAXINT,
						     0,
						     BTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_SIZING,
                                   g_param_spec_enum ("sizing",
                                                      P_("Sizing"),
                                                      P_("Resize mode of the column"),
                                                      BTK_TYPE_TREE_VIEW_COLUMN_SIZING,
                                                      BTK_TREE_VIEW_COLUMN_GROW_ONLY,
                                                      BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_FIXED_WIDTH,
                                   g_param_spec_int ("fixed-width",
                                                     P_("Fixed Width"),
                                                     P_("Current fixed width of the column"),
                                                     1,
                                                     G_MAXINT,
                                                     1, /* not useful */
                                                     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_MIN_WIDTH,
                                   g_param_spec_int ("min-width",
                                                     P_("Minimum Width"),
                                                     P_("Minimum allowed width of the column"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_MAX_WIDTH,
                                   g_param_spec_int ("max-width",
                                                     P_("Maximum Width"),
                                                     P_("Maximum allowed width of the column"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        P_("Title"),
                                                        P_("Title to appear in column header"),
                                                        "",
                                                        BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_EXPAND,
                                   g_param_spec_boolean ("expand",
							 P_("Expand"),
							 P_("Column gets share of extra width allocated to the widget"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_CLICKABLE,
                                   g_param_spec_boolean ("clickable",
                                                        P_("Clickable"),
                                                        P_("Whether the header can be clicked"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));
  

  g_object_class_install_property (object_class,
                                   PROP_WIDGET,
                                   g_param_spec_object ("widget",
                                                        P_("Widget"),
                                                        P_("Widget to put in column header button instead of column title"),
                                                        BTK_TYPE_WIDGET,
                                                        BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_ALIGNMENT,
                                   g_param_spec_float ("alignment",
                                                       P_("Alignment"),
                                                       P_("X Alignment of the column header text or widget"),
                                                       0.0,
                                                       1.0,
                                                       0.0,
                                                       BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
							 P_("Reorderable"),
							 P_("Whether the column can be reordered around the headers"),
							 FALSE,
							 BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SORT_INDICATOR,
                                   g_param_spec_boolean ("sort-indicator",
                                                        P_("Sort indicator"),
                                                        P_("Whether to show a sort indicator"),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_SORT_ORDER,
                                   g_param_spec_enum ("sort-order",
                                                      P_("Sort order"),
                                                      P_("Sort direction the sort indicator should indicate"),
                                                      BTK_TYPE_SORT_TYPE,
                                                      BTK_SORT_ASCENDING,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkTreeViewColumn:sort-column-id:
   *
   * Logical sort column ID this column sorts on when selected for sorting. Setting the sort column ID makes the column header
   * clickable. Set to %-1 to make the column unsortable.
   *
   * Since: 2.18
   **/
  g_object_class_install_property (object_class,
                                   PROP_SORT_COLUMN_ID,
                                   g_param_spec_int ("sort-column-id",
                                                     P_("Sort column ID"),
                                                     P_("Logical sort column ID this column sorts on when selected for sorting"),
                                                     -1,
                                                     G_MAXINT,
                                                     -1,
                                                     BTK_PARAM_READWRITE));
}

static void
btk_tree_view_column_buildable_init (BtkBuildableIface *iface)
{
  iface->add_child = _btk_cell_layout_buildable_add_child;
  iface->custom_tag_start = _btk_cell_layout_buildable_custom_tag_start;
  iface->custom_tag_end = _btk_cell_layout_buildable_custom_tag_end;
}

static void
btk_tree_view_column_cell_layout_init (BtkCellLayoutIface *iface)
{
  iface->pack_start = btk_tree_view_column_cell_layout_pack_start;
  iface->pack_end = btk_tree_view_column_cell_layout_pack_end;
  iface->clear = btk_tree_view_column_cell_layout_clear;
  iface->add_attribute = btk_tree_view_column_cell_layout_add_attribute;
  iface->set_cell_data_func = btk_tree_view_column_cell_layout_set_cell_data_func;
  iface->clear_attributes = btk_tree_view_column_cell_layout_clear_attributes;
  iface->reorder = btk_tree_view_column_cell_layout_reorder;
  iface->get_cells = btk_tree_view_column_cell_layout_get_cells;
}

static void
btk_tree_view_column_init (BtkTreeViewColumn *tree_column)
{
  tree_column->button = NULL;
  tree_column->xalign = 0.0;
  tree_column->width = 0;
  tree_column->spacing = 0;
  tree_column->requested_width = -1;
  tree_column->min_width = -1;
  tree_column->max_width = -1;
  tree_column->resized_width = 0;
  tree_column->column_type = BTK_TREE_VIEW_COLUMN_GROW_ONLY;
  tree_column->visible = TRUE;
  tree_column->resizable = FALSE;
  tree_column->expand = FALSE;
  tree_column->clickable = FALSE;
  tree_column->dirty = TRUE;
  tree_column->sort_order = BTK_SORT_ASCENDING;
  tree_column->show_sort_indicator = FALSE;
  tree_column->property_changed_signal = 0;
  tree_column->sort_clicked_signal = 0;
  tree_column->sort_column_changed_signal = 0;
  tree_column->sort_column_id = -1;
  tree_column->reorderable = FALSE;
  tree_column->maybe_reordered = FALSE;
  tree_column->fixed_width = 1;
  tree_column->use_resized_width = FALSE;
  tree_column->title = g_strdup ("");
}

static void
btk_tree_view_column_finalize (GObject *object)
{
  BtkTreeViewColumn *tree_column = (BtkTreeViewColumn *) object;
  GList *list;

  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) list->data;

      if (info->destroy)
	{
	  GDestroyNotify d = info->destroy;

	  info->destroy = NULL;
	  d (info->func_data);
	}
      btk_tree_view_column_clear_attributes_by_info (tree_column, info);
      g_object_unref (info->cell);
      g_free (info);
    }

  g_free (tree_column->title);
  g_list_free (tree_column->cell_list);

  if (tree_column->child)
    g_object_unref (tree_column->child);

  G_OBJECT_CLASS (btk_tree_view_column_parent_class)->finalize (object);
}

static void
btk_tree_view_column_set_property (GObject         *object,
                                   guint            prop_id,
                                   const GValue    *value,
                                   GParamSpec      *pspec)
{
  BtkTreeViewColumn *tree_column;

  tree_column = BTK_TREE_VIEW_COLUMN (object);

  switch (prop_id)
    {
    case PROP_VISIBLE:
      btk_tree_view_column_set_visible (tree_column,
                                        g_value_get_boolean (value));
      break;

    case PROP_RESIZABLE:
      btk_tree_view_column_set_resizable (tree_column,
					  g_value_get_boolean (value));
      break;

    case PROP_SIZING:
      btk_tree_view_column_set_sizing (tree_column,
                                       g_value_get_enum (value));
      break;

    case PROP_FIXED_WIDTH:
      btk_tree_view_column_set_fixed_width (tree_column,
					    g_value_get_int (value));
      break;

    case PROP_MIN_WIDTH:
      btk_tree_view_column_set_min_width (tree_column,
                                          g_value_get_int (value));
      break;

    case PROP_MAX_WIDTH:
      btk_tree_view_column_set_max_width (tree_column,
                                          g_value_get_int (value));
      break;

    case PROP_SPACING:
      btk_tree_view_column_set_spacing (tree_column,
					g_value_get_int (value));
      break;

    case PROP_TITLE:
      btk_tree_view_column_set_title (tree_column,
                                      g_value_get_string (value));
      break;

    case PROP_EXPAND:
      btk_tree_view_column_set_expand (tree_column,
				       g_value_get_boolean (value));
      break;

    case PROP_CLICKABLE:
      btk_tree_view_column_set_clickable (tree_column,
                                          g_value_get_boolean (value));
      break;

    case PROP_WIDGET:
      btk_tree_view_column_set_widget (tree_column,
                                       (BtkWidget*) g_value_get_object (value));
      break;

    case PROP_ALIGNMENT:
      btk_tree_view_column_set_alignment (tree_column,
                                          g_value_get_float (value));
      break;

    case PROP_REORDERABLE:
      btk_tree_view_column_set_reorderable (tree_column,
					    g_value_get_boolean (value));
      break;

    case PROP_SORT_INDICATOR:
      btk_tree_view_column_set_sort_indicator (tree_column,
                                               g_value_get_boolean (value));
      break;

    case PROP_SORT_ORDER:
      btk_tree_view_column_set_sort_order (tree_column,
                                           g_value_get_enum (value));
      break;
      
    case PROP_SORT_COLUMN_ID:
      btk_tree_view_column_set_sort_column_id (tree_column,
                                               g_value_get_int (value));
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_tree_view_column_get_property (GObject         *object,
                                   guint            prop_id,
                                   GValue          *value,
                                   GParamSpec      *pspec)
{
  BtkTreeViewColumn *tree_column;

  tree_column = BTK_TREE_VIEW_COLUMN (object);

  switch (prop_id)
    {
    case PROP_VISIBLE:
      g_value_set_boolean (value,
                           btk_tree_view_column_get_visible (tree_column));
      break;

    case PROP_RESIZABLE:
      g_value_set_boolean (value,
                           btk_tree_view_column_get_resizable (tree_column));
      break;

    case PROP_WIDTH:
      g_value_set_int (value,
                       btk_tree_view_column_get_width (tree_column));
      break;

    case PROP_SPACING:
      g_value_set_int (value,
                       btk_tree_view_column_get_spacing (tree_column));
      break;

    case PROP_SIZING:
      g_value_set_enum (value,
                        btk_tree_view_column_get_sizing (tree_column));
      break;

    case PROP_FIXED_WIDTH:
      g_value_set_int (value,
                       btk_tree_view_column_get_fixed_width (tree_column));
      break;

    case PROP_MIN_WIDTH:
      g_value_set_int (value,
                       btk_tree_view_column_get_min_width (tree_column));
      break;

    case PROP_MAX_WIDTH:
      g_value_set_int (value,
                       btk_tree_view_column_get_max_width (tree_column));
      break;

    case PROP_TITLE:
      g_value_set_string (value,
                          btk_tree_view_column_get_title (tree_column));
      break;

    case PROP_EXPAND:
      g_value_set_boolean (value,
                          btk_tree_view_column_get_expand (tree_column));
      break;

    case PROP_CLICKABLE:
      g_value_set_boolean (value,
                           btk_tree_view_column_get_clickable (tree_column));
      break;

    case PROP_WIDGET:
      g_value_set_object (value,
                          (GObject*) btk_tree_view_column_get_widget (tree_column));
      break;

    case PROP_ALIGNMENT:
      g_value_set_float (value,
                         btk_tree_view_column_get_alignment (tree_column));
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value,
			   btk_tree_view_column_get_reorderable (tree_column));
      break;

    case PROP_SORT_INDICATOR:
      g_value_set_boolean (value,
                           btk_tree_view_column_get_sort_indicator (tree_column));
      break;

    case PROP_SORT_ORDER:
      g_value_set_enum (value,
                        btk_tree_view_column_get_sort_order (tree_column));
      break;
      
    case PROP_SORT_COLUMN_ID:
      g_value_set_int (value,
                       btk_tree_view_column_get_sort_column_id (tree_column));
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Implementation of BtkCellLayout interface
 */

static void
btk_tree_view_column_cell_layout_pack_start (BtkCellLayout   *cell_layout,
                                             BtkCellRenderer *cell,
                                             gboolean         expand)
{
  BtkTreeViewColumn *column;
  BtkTreeViewColumnCellInfo *cell_info;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  column = BTK_TREE_VIEW_COLUMN (cell_layout);
  g_return_if_fail (! btk_tree_view_column_get_cell_info (column, cell));

  g_object_ref_sink (cell);

  cell_info = g_new0 (BtkTreeViewColumnCellInfo, 1);
  cell_info->cell = cell;
  cell_info->expand = expand ? TRUE : FALSE;
  cell_info->pack = BTK_PACK_START;
  cell_info->has_focus = 0;
  cell_info->attributes = NULL;

  column->cell_list = g_list_append (column->cell_list, cell_info);
}

static void
btk_tree_view_column_cell_layout_pack_end (BtkCellLayout   *cell_layout,
                                           BtkCellRenderer *cell,
                                           gboolean         expand)
{
  BtkTreeViewColumn *column;
  BtkTreeViewColumnCellInfo *cell_info;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  column = BTK_TREE_VIEW_COLUMN (cell_layout);
  g_return_if_fail (! btk_tree_view_column_get_cell_info (column, cell));

  g_object_ref_sink (cell);

  cell_info = g_new0 (BtkTreeViewColumnCellInfo, 1);
  cell_info->cell = cell;
  cell_info->expand = expand ? TRUE : FALSE;
  cell_info->pack = BTK_PACK_END;
  cell_info->has_focus = 0;
  cell_info->attributes = NULL;

  column->cell_list = g_list_append (column->cell_list, cell_info);
}

static void
btk_tree_view_column_cell_layout_clear (BtkCellLayout *cell_layout)
{
  BtkTreeViewColumn *column;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  column = BTK_TREE_VIEW_COLUMN (cell_layout);

  while (column->cell_list)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *)column->cell_list->data;

      btk_tree_view_column_cell_layout_clear_attributes (cell_layout, info->cell);
      g_object_unref (info->cell);
      g_free (info);
      column->cell_list = g_list_delete_link (column->cell_list, 
					      column->cell_list);
    }
}

static void
btk_tree_view_column_cell_layout_add_attribute (BtkCellLayout   *cell_layout,
                                                BtkCellRenderer *cell,
                                                const gchar     *attribute,
                                                gint             column)
{
  BtkTreeViewColumn *tree_column;
  BtkTreeViewColumnCellInfo *info;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  tree_column = BTK_TREE_VIEW_COLUMN (cell_layout);

  info = btk_tree_view_column_get_cell_info (tree_column, cell);
  g_return_if_fail (info != NULL);

  info->attributes = g_slist_prepend (info->attributes, GINT_TO_POINTER (column));
  info->attributes = g_slist_prepend (info->attributes, g_strdup (attribute));

  if (tree_column->tree_view)
    _btk_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

static void
btk_tree_view_column_cell_layout_set_cell_data_func (BtkCellLayout         *cell_layout,
                                                     BtkCellRenderer       *cell,
                                                     BtkCellLayoutDataFunc  func,
                                                     gpointer               func_data,
                                                     GDestroyNotify         destroy)
{
  BtkTreeViewColumn *column;
  BtkTreeViewColumnCellInfo *info;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  column = BTK_TREE_VIEW_COLUMN (cell_layout);

  info = btk_tree_view_column_get_cell_info (column, cell);
  g_return_if_fail (info != NULL);

  if (info->destroy)
    {
      GDestroyNotify d = info->destroy;

      info->destroy = NULL;
      d (info->func_data);
    }

  info->func = (BtkTreeCellDataFunc)func;
  info->func_data = func_data;
  info->destroy = destroy;

  if (column->tree_view)
    _btk_tree_view_column_cell_set_dirty (column, TRUE);
}

static void
btk_tree_view_column_cell_layout_clear_attributes (BtkCellLayout    *cell_layout,
                                                   BtkCellRenderer  *cell_renderer)
{
  BtkTreeViewColumn *column;
  BtkTreeViewColumnCellInfo *info;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  column = BTK_TREE_VIEW_COLUMN (cell_layout);

  info = btk_tree_view_column_get_cell_info (column, cell_renderer);
  if (info)
    btk_tree_view_column_clear_attributes_by_info (column, info);
}

static void
btk_tree_view_column_cell_layout_reorder (BtkCellLayout   *cell_layout,
                                          BtkCellRenderer *cell,
                                          gint             position)
{
  GList *link;
  BtkTreeViewColumn *column;
  BtkTreeViewColumnCellInfo *info;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (cell_layout));
  column = BTK_TREE_VIEW_COLUMN (cell_layout);

  info = btk_tree_view_column_get_cell_info (column, cell);

  g_return_if_fail (info != NULL);
  g_return_if_fail (position >= 0);

  link = g_list_find (column->cell_list, info);

  g_return_if_fail (link != NULL);

  column->cell_list = g_list_delete_link (column->cell_list, link);
  column->cell_list = g_list_insert (column->cell_list, info, position);

  if (column->tree_view)
    btk_widget_queue_draw (column->tree_view);
}

static void
btk_tree_view_column_clear_attributes_by_info (BtkTreeViewColumn *tree_column,
					       BtkTreeViewColumnCellInfo *info)
{
  GSList *list;

  list = info->attributes;

  while (list && list->next)
    {
      g_free (list->data);
      list = list->next->next;
    }
  g_slist_free (info->attributes);
  info->attributes = NULL;

  if (tree_column->tree_view)
    _btk_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

/* Helper functions
 */

/* Button handling code
 */
static void
btk_tree_view_column_create_button (BtkTreeViewColumn *tree_column)
{
  BtkTreeView *tree_view;
  BtkWidget *child;
  BtkWidget *hbox;

  tree_view = (BtkTreeView *) tree_column->tree_view;

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (tree_column->button == NULL);

  btk_widget_push_composite_child ();
  tree_column->button = btk_button_new ();
  btk_widget_add_events (tree_column->button, BDK_POINTER_MOTION_MASK);
  btk_widget_pop_composite_child ();

  /* make sure we own a reference to it as well. */
  if (tree_view->priv->header_window)
    btk_widget_set_parent_window (tree_column->button, tree_view->priv->header_window);
  btk_widget_set_parent (tree_column->button, BTK_WIDGET (tree_view));

  g_signal_connect (tree_column->button, "event",
		    G_CALLBACK (btk_tree_view_column_button_event),
		    tree_column);
  g_signal_connect (tree_column->button, "clicked",
		    G_CALLBACK (btk_tree_view_column_button_clicked),
		    tree_column);

  tree_column->alignment = btk_alignment_new (tree_column->xalign, 0.5, 0.0, 0.0);

  hbox = btk_hbox_new (FALSE, 2);
  tree_column->arrow = btk_arrow_new (BTK_ARROW_DOWN, BTK_SHADOW_IN);

  if (tree_column->child)
    child = tree_column->child;
  else
    {
      child = btk_label_new (tree_column->title);
      btk_widget_show (child);
    }

  g_signal_connect (child, "mnemonic-activate",
		    G_CALLBACK (btk_tree_view_column_mnemonic_activate),
		    tree_column);

  if (tree_column->xalign <= 0.5)
    btk_box_pack_end (BTK_BOX (hbox), tree_column->arrow, FALSE, FALSE, 0);
  else
    btk_box_pack_start (BTK_BOX (hbox), tree_column->arrow, FALSE, FALSE, 0);

  btk_box_pack_start (BTK_BOX (hbox), tree_column->alignment, TRUE, TRUE, 0);
        
  btk_container_add (BTK_CONTAINER (tree_column->alignment), child);
  btk_container_add (BTK_CONTAINER (tree_column->button), hbox);

  btk_widget_show (hbox);
  btk_widget_show (tree_column->alignment);
  btk_tree_view_column_update_button (tree_column);
}

static void 
btk_tree_view_column_update_button (BtkTreeViewColumn *tree_column)
{
  gint sort_column_id = -1;
  BtkWidget *hbox;
  BtkWidget *alignment;
  BtkWidget *arrow;
  BtkWidget *current_child;
  BtkArrowType arrow_type = BTK_ARROW_NONE;
  BtkTreeModel *model;

  if (tree_column->tree_view)
    model = btk_tree_view_get_model (BTK_TREE_VIEW (tree_column->tree_view));
  else
    model = NULL;

  /* Create a button if necessary */
  if (tree_column->visible &&
      tree_column->button == NULL &&
      tree_column->tree_view &&
      btk_widget_get_realized (tree_column->tree_view))
    btk_tree_view_column_create_button (tree_column);
  
  if (! tree_column->button)
    return;

  hbox = BTK_BIN (tree_column->button)->child;
  alignment = tree_column->alignment;
  arrow = tree_column->arrow;
  current_child = BTK_BIN (alignment)->child;

  /* Set up the actual button */
  btk_alignment_set (BTK_ALIGNMENT (alignment), tree_column->xalign,
		     0.5, 0.0, 0.0);
      
  if (tree_column->child)
    {
      if (current_child != tree_column->child)
	{
	  btk_container_remove (BTK_CONTAINER (alignment),
				current_child);
	  btk_container_add (BTK_CONTAINER (alignment),
			     tree_column->child);
	}
    }
  else 
    {
      if (current_child == NULL)
	{
	  current_child = btk_label_new (NULL);
	  btk_widget_show (current_child);
	  btk_container_add (BTK_CONTAINER (alignment),
			     current_child);
	}

      g_return_if_fail (BTK_IS_LABEL (current_child));

      if (tree_column->title)
	btk_label_set_text_with_mnemonic (BTK_LABEL (current_child),
					  tree_column->title);
      else
	btk_label_set_text_with_mnemonic (BTK_LABEL (current_child),
					  "");
    }

  if (BTK_IS_TREE_SORTABLE (model))
    btk_tree_sortable_get_sort_column_id (BTK_TREE_SORTABLE (model),
					  &sort_column_id,
					  NULL);

  if (tree_column->show_sort_indicator)
    {
      gboolean alternative;

      g_object_get (btk_widget_get_settings (tree_column->tree_view),
		    "btk-alternative-sort-arrows", &alternative,
		    NULL);

      switch (tree_column->sort_order)
        {
	  case BTK_SORT_ASCENDING:
	    arrow_type = alternative ? BTK_ARROW_UP : BTK_ARROW_DOWN;
	    break;

	  case BTK_SORT_DESCENDING:
	    arrow_type = alternative ? BTK_ARROW_DOWN : BTK_ARROW_UP;
	    break;

	  default:
	    g_warning (G_STRLOC": bad sort order");
	    break;
	}
    }

  btk_arrow_set (BTK_ARROW (arrow),
		 arrow_type,
		 BTK_SHADOW_IN);

  /* Put arrow on the right if the text is left-or-center justified, and on the
   * left otherwise; do this by packing boxes, so flipping text direction will
   * reverse things
   */
  g_object_ref (arrow);
  btk_container_remove (BTK_CONTAINER (hbox), arrow);

  if (tree_column->xalign <= 0.5)
    {
      btk_box_pack_end (BTK_BOX (hbox), arrow, FALSE, FALSE, 0);
    }
  else
    {
      btk_box_pack_start (BTK_BOX (hbox), arrow, FALSE, FALSE, 0);
      /* move it to the front */
      btk_box_reorder_child (BTK_BOX (hbox), arrow, 0);
    }
  g_object_unref (arrow);

  if (tree_column->show_sort_indicator
      || (BTK_IS_TREE_SORTABLE (model) && tree_column->sort_column_id >= 0))
    btk_widget_show (arrow);
  else
    btk_widget_hide (arrow);

  /* It's always safe to hide the button.  It isn't always safe to show it, as
   * if you show it before it's realized, it'll get the wrong window. */
  if (tree_column->button &&
      tree_column->tree_view != NULL &&
      btk_widget_get_realized (tree_column->tree_view))
    {
      if (tree_column->visible)
	{
	  btk_widget_show_now (tree_column->button);
	  if (tree_column->window)
	    {
	      if (tree_column->resizable)
		{
		  bdk_window_show (tree_column->window);
		  bdk_window_raise (tree_column->window);
		}
	      else
		{
		  bdk_window_hide (tree_column->window);
		}
	    }
	}
      else
	{
	  btk_widget_hide (tree_column->button);
	  if (tree_column->window)
	    bdk_window_hide (tree_column->window);
	}
    }
  
  if (tree_column->reorderable || tree_column->clickable)
    {
      btk_widget_set_can_focus (tree_column->button, TRUE);
    }
  else
    {
      btk_widget_set_can_focus (tree_column->button, FALSE);
      if (btk_widget_has_focus (tree_column->button))
	{
	  BtkWidget *toplevel = btk_widget_get_toplevel (tree_column->tree_view);
	  if (btk_widget_is_toplevel (toplevel))
	    {
	      btk_window_set_focus (BTK_WINDOW (toplevel), NULL);
	    }
	}
    }
  /* Queue a resize on the assumption that we always want to catch all changes
   * and columns don't change all that often.
   */
  if (btk_widget_get_realized (tree_column->tree_view))
     btk_widget_queue_resize (tree_column->tree_view);

}

/* Button signal handlers
 */

static gint
btk_tree_view_column_button_event (BtkWidget *widget,
				   BdkEvent  *event,
				   gpointer   data)
{
  BtkTreeViewColumn *column = (BtkTreeViewColumn *) data;

  g_return_val_if_fail (event != NULL, FALSE);

  if (event->type == BDK_BUTTON_PRESS &&
      column->reorderable &&
      ((BdkEventButton *)event)->button == 1)
    {
      column->maybe_reordered = TRUE;
      bdk_window_get_pointer (BTK_BUTTON (widget)->event_window,
			      &column->drag_x,
			      &column->drag_y,
			      NULL);
      btk_widget_grab_focus (widget);
    }

  if (event->type == BDK_BUTTON_RELEASE ||
      event->type == BDK_LEAVE_NOTIFY)
    column->maybe_reordered = FALSE;
  
  if (event->type == BDK_MOTION_NOTIFY &&
      column->maybe_reordered &&
      (btk_drag_check_threshold (widget,
				 column->drag_x,
				 column->drag_y,
				 (gint) ((BdkEventMotion *)event)->x,
				 (gint) ((BdkEventMotion *)event)->y)))
    {
      column->maybe_reordered = FALSE;
      _btk_tree_view_column_start_drag (BTK_TREE_VIEW (column->tree_view), column);
      return TRUE;
    }
  if (column->clickable == FALSE)
    {
      switch (event->type)
	{
	case BDK_BUTTON_PRESS:
	case BDK_2BUTTON_PRESS:
	case BDK_3BUTTON_PRESS:
	case BDK_MOTION_NOTIFY:
	case BDK_BUTTON_RELEASE:
	case BDK_ENTER_NOTIFY:
	case BDK_LEAVE_NOTIFY:
	  return TRUE;
	default:
	  return FALSE;
	}
    }
  return FALSE;
}


static void
btk_tree_view_column_button_clicked (BtkWidget *widget, gpointer data)
{
  g_signal_emit_by_name (data, "clicked");
}

static gboolean
btk_tree_view_column_mnemonic_activate (BtkWidget *widget,
					gboolean   group_cycling,
					gpointer   data)
{
  BtkTreeViewColumn *column = (BtkTreeViewColumn *)data;

  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (column), FALSE);

  BTK_TREE_VIEW (column->tree_view)->priv->focus_column = column;
  if (column->clickable)
    btk_button_clicked (BTK_BUTTON (column->button));
  else if (btk_widget_get_can_focus (column->button))
    btk_widget_grab_focus (column->button);
  else
    btk_widget_grab_focus (column->tree_view);

  return TRUE;
}

static void
btk_tree_view_model_sort_column_changed (BtkTreeSortable   *sortable,
					 BtkTreeViewColumn *column)
{
  gint sort_column_id;
  BtkSortType order;

  if (btk_tree_sortable_get_sort_column_id (sortable,
					    &sort_column_id,
					    &order))
    {
      if (sort_column_id == column->sort_column_id)
	{
	  btk_tree_view_column_set_sort_indicator (column, TRUE);
	  btk_tree_view_column_set_sort_order (column, order);
	}
      else
	{
	  btk_tree_view_column_set_sort_indicator (column, FALSE);
	}
    }
  else
    {
      btk_tree_view_column_set_sort_indicator (column, FALSE);
    }
}

static void
btk_tree_view_column_sort (BtkTreeViewColumn *tree_column,
			   gpointer           data)
{
  gint sort_column_id;
  BtkSortType order;
  gboolean has_sort_column;
  gboolean has_default_sort_func;

  g_return_if_fail (tree_column->tree_view != NULL);

  has_sort_column =
    btk_tree_sortable_get_sort_column_id (BTK_TREE_SORTABLE (BTK_TREE_VIEW (tree_column->tree_view)->priv->model),
					  &sort_column_id,
					  &order);
  has_default_sort_func =
    btk_tree_sortable_has_default_sort_func (BTK_TREE_SORTABLE (BTK_TREE_VIEW (tree_column->tree_view)->priv->model));

  if (has_sort_column &&
      sort_column_id == tree_column->sort_column_id)
    {
      if (order == BTK_SORT_ASCENDING)
	btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (BTK_TREE_VIEW (tree_column->tree_view)->priv->model),
					      tree_column->sort_column_id,
					      BTK_SORT_DESCENDING);
      else if (order == BTK_SORT_DESCENDING && has_default_sort_func)
	btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (BTK_TREE_VIEW (tree_column->tree_view)->priv->model),
					      BTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      BTK_SORT_ASCENDING);
      else
	btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (BTK_TREE_VIEW (tree_column->tree_view)->priv->model),
					      tree_column->sort_column_id,
					      BTK_SORT_ASCENDING);
    }
  else
    {
      btk_tree_sortable_set_sort_column_id (BTK_TREE_SORTABLE (BTK_TREE_VIEW (tree_column->tree_view)->priv->model),
					    tree_column->sort_column_id,
					    BTK_SORT_ASCENDING);
    }
}


static void
btk_tree_view_column_setup_sort_column_id_callback (BtkTreeViewColumn *tree_column)
{
  BtkTreeModel *model;

  if (tree_column->tree_view == NULL)
    return;

  model = btk_tree_view_get_model (BTK_TREE_VIEW (tree_column->tree_view));

  if (model == NULL)
    return;

  if (BTK_IS_TREE_SORTABLE (model) &&
      tree_column->sort_column_id != -1)
    {
      gint real_sort_column_id;
      BtkSortType real_order;

      if (tree_column->sort_column_changed_signal == 0)
        tree_column->sort_column_changed_signal =
	  g_signal_connect (model, "sort-column-changed",
			    G_CALLBACK (btk_tree_view_model_sort_column_changed),
			    tree_column);
      
      if (btk_tree_sortable_get_sort_column_id (BTK_TREE_SORTABLE (model),
						&real_sort_column_id,
						&real_order) &&
	  (real_sort_column_id == tree_column->sort_column_id))
	{
	  btk_tree_view_column_set_sort_indicator (tree_column, TRUE);
	  btk_tree_view_column_set_sort_order (tree_column, real_order);
 	}
      else 
	{
	  btk_tree_view_column_set_sort_indicator (tree_column, FALSE);
	}
   }
}


/* Exported Private Functions.
 * These should only be called by btktreeview.c or btktreeviewcolumn.c
 */

void
_btk_tree_view_column_realize_button (BtkTreeViewColumn *column)
{
  BtkTreeView *tree_view;
  BdkWindowAttr attr;
  guint attributes_mask;
  gboolean rtl;

  tree_view = (BtkTreeView *)column->tree_view;
  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_view)) == BTK_TEXT_DIR_RTL);

  g_return_if_fail (BTK_IS_TREE_VIEW (tree_view));
  g_return_if_fail (btk_widget_get_realized (BTK_WIDGET (tree_view)));
  g_return_if_fail (tree_view->priv->header_window != NULL);
  g_return_if_fail (column->button != NULL);

  btk_widget_set_parent_window (column->button, tree_view->priv->header_window);

  if (column->visible)
    btk_widget_show (column->button);

  attr.window_type = BDK_WINDOW_CHILD;
  attr.wclass = BDK_INPUT_ONLY;
  attr.visual = btk_widget_get_visual (BTK_WIDGET (tree_view));
  attr.colormap = btk_widget_get_colormap (BTK_WIDGET (tree_view));
  attr.event_mask = btk_widget_get_events (BTK_WIDGET (tree_view)) |
                    (BDK_BUTTON_PRESS_MASK |
		     BDK_BUTTON_RELEASE_MASK |
		     BDK_POINTER_MOTION_MASK |
		     BDK_POINTER_MOTION_HINT_MASK |
		     BDK_KEY_PRESS_MASK);
  attributes_mask = BDK_WA_CURSOR | BDK_WA_X | BDK_WA_Y;
  attr.cursor = bdk_cursor_new_for_display (bdk_window_get_display (tree_view->priv->header_window),
					    BDK_SB_H_DOUBLE_ARROW);
  attr.y = 0;
  attr.width = TREE_VIEW_DRAG_WIDTH;
  attr.height = tree_view->priv->header_height;

  attr.x = (column->button->allocation.x + (rtl ? 0 : column->button->allocation.width)) - TREE_VIEW_DRAG_WIDTH / 2;
  column->window = bdk_window_new (tree_view->priv->header_window,
				   &attr, attributes_mask);
  bdk_window_set_user_data (column->window, tree_view);

  btk_tree_view_column_update_button (column);

  bdk_cursor_unref (attr.cursor);
}

void
_btk_tree_view_column_unrealize_button (BtkTreeViewColumn *column)
{
  g_return_if_fail (column != NULL);
  g_return_if_fail (column->window != NULL);

  bdk_window_set_user_data (column->window, NULL);
  bdk_window_destroy (column->window);
  column->window = NULL;
}

void
_btk_tree_view_column_unset_model (BtkTreeViewColumn *column,
				   BtkTreeModel      *old_model)
{
  if (column->sort_column_changed_signal)
    {
      g_signal_handler_disconnect (old_model,
				   column->sort_column_changed_signal);
      column->sort_column_changed_signal = 0;
    }
  btk_tree_view_column_set_sort_indicator (column, FALSE);
}

void
_btk_tree_view_column_set_tree_view (BtkTreeViewColumn *column,
				     BtkTreeView       *tree_view)
{
  g_assert (column->tree_view == NULL);

  column->tree_view = BTK_WIDGET (tree_view);
  btk_tree_view_column_create_button (column);

  column->property_changed_signal =
	  g_signal_connect_swapped (tree_view,
				    "notify::model",
				    G_CALLBACK (btk_tree_view_column_setup_sort_column_id_callback),
				    column);

  btk_tree_view_column_setup_sort_column_id_callback (column);
}

void
_btk_tree_view_column_unset_tree_view (BtkTreeViewColumn *column)
{
  if (column->tree_view && column->button)
    {
      btk_container_remove (BTK_CONTAINER (column->tree_view), column->button);
    }
  if (column->property_changed_signal)
    {
      g_signal_handler_disconnect (column->tree_view, column->property_changed_signal);
      column->property_changed_signal = 0;
    }

  if (column->sort_column_changed_signal)
    {
      g_signal_handler_disconnect (btk_tree_view_get_model (BTK_TREE_VIEW (column->tree_view)),
				   column->sort_column_changed_signal);
      column->sort_column_changed_signal = 0;
    }

  column->tree_view = NULL;
  column->button = NULL;
}

gboolean
_btk_tree_view_column_has_editable_cell (BtkTreeViewColumn *column)
{
  GList *list;

  for (list = column->cell_list; list; list = list->next)
    if (((BtkTreeViewColumnCellInfo *)list->data)->cell->mode ==
	BTK_CELL_RENDERER_MODE_EDITABLE)
      return TRUE;

  return FALSE;
}

/* gets cell being edited */
BtkCellRenderer *
_btk_tree_view_column_get_edited_cell (BtkTreeViewColumn *column)
{
  GList *list;

  for (list = column->cell_list; list; list = list->next)
    if (((BtkTreeViewColumnCellInfo *)list->data)->in_editing_mode)
      return ((BtkTreeViewColumnCellInfo *)list->data)->cell;

  return NULL;
}

gint
_btk_tree_view_column_count_special_cells (BtkTreeViewColumn *column)
{
  gint i = 0;
  GList *list;

  for (list = column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *cellinfo = list->data;

      if ((cellinfo->cell->mode == BTK_CELL_RENDERER_MODE_EDITABLE ||
	  cellinfo->cell->mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE) &&
	  cellinfo->cell->visible)
	i++;
    }

  return i;
}

BtkCellRenderer *
_btk_tree_view_column_get_cell_at_pos (BtkTreeViewColumn *column,
				       gint               x)
{
  GList *list;
  gint current_x = 0;

  list = btk_tree_view_column_cell_first (column);
  for (; list; list = btk_tree_view_column_cell_next (column, list))
   {
     BtkTreeViewColumnCellInfo *cellinfo = list->data;
     if (current_x <= x && x <= current_x + cellinfo->real_width)
       return cellinfo->cell;
     current_x += cellinfo->real_width;
   }

  return NULL;
}

/* Public Functions */


/**
 * btk_tree_view_column_new:
 * 
 * Creates a new #BtkTreeViewColumn.
 * 
 * Return value: A newly created #BtkTreeViewColumn.
 **/
BtkTreeViewColumn *
btk_tree_view_column_new (void)
{
  BtkTreeViewColumn *tree_column;

  tree_column = g_object_new (BTK_TYPE_TREE_VIEW_COLUMN, NULL);

  return tree_column;
}

/**
 * btk_tree_view_column_new_with_attributes:
 * @title: The title to set the header to.
 * @cell: The #BtkCellRenderer.
 * @Varargs: A %NULL-terminated list of attributes.
 * 
 * Creates a new #BtkTreeViewColumn with a number of default values.  This is
 * equivalent to calling btk_tree_view_column_set_title(),
 * btk_tree_view_column_pack_start(), and
 * btk_tree_view_column_set_attributes() on the newly created #BtkTreeViewColumn.
 *
 * Here's a simple example:
 * |[
 *  enum { TEXT_COLUMN, COLOR_COLUMN, N_COLUMNS };
 *  ...
 *  {
 *    BtkTreeViewColumn *column;
 *    BtkCellRenderer   *renderer = btk_cell_renderer_text_new ();
 *  
 *    column = btk_tree_view_column_new_with_attributes ("Title",
 *                                                       renderer,
 *                                                       "text", TEXT_COLUMN,
 *                                                       "foreground", COLOR_COLUMN,
 *                                                       NULL);
 *  }
 * ]|
 * 
 * Return value: A newly created #BtkTreeViewColumn.
 **/
BtkTreeViewColumn *
btk_tree_view_column_new_with_attributes (const gchar     *title,
					  BtkCellRenderer *cell,
					  ...)
{
  BtkTreeViewColumn *retval;
  va_list args;

  retval = btk_tree_view_column_new ();

  btk_tree_view_column_set_title (retval, title);
  btk_tree_view_column_pack_start (retval, cell, TRUE);

  va_start (args, cell);
  btk_tree_view_column_set_attributesv (retval, cell, args);
  va_end (args);

  return retval;
}

static BtkTreeViewColumnCellInfo *
btk_tree_view_column_get_cell_info (BtkTreeViewColumn *tree_column,
				    BtkCellRenderer   *cell_renderer)
{
  GList *list;
  for (list = tree_column->cell_list; list; list = list->next)
    if (((BtkTreeViewColumnCellInfo *)list->data)->cell == cell_renderer)
      return (BtkTreeViewColumnCellInfo *) list->data;
  return NULL;
}


/**
 * btk_tree_view_column_pack_start:
 * @tree_column: A #BtkTreeViewColumn.
 * @cell: The #BtkCellRenderer. 
 * @expand: %TRUE if @cell is to be given extra space allocated to @tree_column.
 *
 * Packs the @cell into the beginning of the column. If @expand is %FALSE, then
 * the @cell is allocated no more space than it needs. Any unused space is divided
 * evenly between cells for which @expand is %TRUE.
 **/
void
btk_tree_view_column_pack_start (BtkTreeViewColumn *tree_column,
				 BtkCellRenderer   *cell,
				 gboolean           expand)
{
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (tree_column), cell, expand);
}

/**
 * btk_tree_view_column_pack_end:
 * @tree_column: A #BtkTreeViewColumn.
 * @cell: The #BtkCellRenderer. 
 * @expand: %TRUE if @cell is to be given extra space allocated to @tree_column.
 *
 * Adds the @cell to end of the column. If @expand is %FALSE, then the @cell
 * is allocated no more space than it needs. Any unused space is divided
 * evenly between cells for which @expand is %TRUE.
 **/
void
btk_tree_view_column_pack_end (BtkTreeViewColumn  *tree_column,
			       BtkCellRenderer    *cell,
			       gboolean            expand)
{
  btk_cell_layout_pack_end (BTK_CELL_LAYOUT (tree_column), cell, expand);
}

/**
 * btk_tree_view_column_clear:
 * @tree_column: A #BtkTreeViewColumn
 * 
 * Unsets all the mappings on all renderers on the @tree_column.
 **/
void
btk_tree_view_column_clear (BtkTreeViewColumn *tree_column)
{
  btk_cell_layout_clear (BTK_CELL_LAYOUT (tree_column));
}

static GList *
btk_tree_view_column_cell_layout_get_cells (BtkCellLayout *layout)
{
  BtkTreeViewColumn *tree_column = BTK_TREE_VIEW_COLUMN (layout);
  GList *retval = NULL, *list;

  g_return_val_if_fail (tree_column != NULL, NULL);

  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *)list->data;

      retval = g_list_append (retval, info->cell);
    }

  return retval;
}

/**
 * btk_tree_view_column_get_cell_renderers:
 * @tree_column: A #BtkTreeViewColumn
 *
 * Returns a newly-allocated #GList of all the cell renderers in the column,
 * in no particular order.  The list must be freed with g_list_free().
 *
 * Return value: A list of #BtkCellRenderers
 *
 * Deprecated: 2.18: use btk_cell_layout_get_cells() instead.
 **/
GList *
btk_tree_view_column_get_cell_renderers (BtkTreeViewColumn *tree_column)
{
  return btk_tree_view_column_cell_layout_get_cells (BTK_CELL_LAYOUT (tree_column));
}

/**
 * btk_tree_view_column_add_attribute:
 * @tree_column: A #BtkTreeViewColumn.
 * @cell_renderer: the #BtkCellRenderer to set attributes on
 * @attribute: An attribute on the renderer
 * @column: The column position on the model to get the attribute from.
 * 
 * Adds an attribute mapping to the list in @tree_column.  The @column is the
 * column of the model to get a value from, and the @attribute is the
 * parameter on @cell_renderer to be set from the value. So for example
 * if column 2 of the model contains strings, you could have the
 * "text" attribute of a #BtkCellRendererText get its values from
 * column 2.
 **/
void
btk_tree_view_column_add_attribute (BtkTreeViewColumn *tree_column,
				    BtkCellRenderer   *cell_renderer,
				    const gchar       *attribute,
				    gint               column)
{
  btk_cell_layout_add_attribute (BTK_CELL_LAYOUT (tree_column),
                                 cell_renderer, attribute, column);
}

static void
btk_tree_view_column_set_attributesv (BtkTreeViewColumn *tree_column,
				      BtkCellRenderer   *cell_renderer,
				      va_list            args)
{
  gchar *attribute;
  gint column;

  attribute = va_arg (args, gchar *);

  btk_tree_view_column_clear_attributes (tree_column, cell_renderer);
  
  while (attribute != NULL)
    {
      column = va_arg (args, gint);
      btk_tree_view_column_add_attribute (tree_column, cell_renderer, attribute, column);
      attribute = va_arg (args, gchar *);
    }
}

/**
 * btk_tree_view_column_set_attributes:
 * @tree_column: A #BtkTreeViewColumn.
 * @cell_renderer: the #BtkCellRenderer we're setting the attributes of
 * @Varargs: A %NULL-terminated list of attributes.
 * 
 * Sets the attributes in the list as the attributes of @tree_column.
 * The attributes should be in attribute/column order, as in
 * btk_tree_view_column_add_attribute(). All existing attributes
 * are removed, and replaced with the new attributes.
 **/
void
btk_tree_view_column_set_attributes (BtkTreeViewColumn *tree_column,
				     BtkCellRenderer   *cell_renderer,
				     ...)
{
  va_list args;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell_renderer));
  g_return_if_fail (btk_tree_view_column_get_cell_info (tree_column, cell_renderer));

  va_start (args, cell_renderer);
  btk_tree_view_column_set_attributesv (tree_column, cell_renderer, args);
  va_end (args);
}


/**
 * btk_tree_view_column_set_cell_data_func:
 * @tree_column: A #BtkTreeViewColumn
 * @cell_renderer: A #BtkCellRenderer
 * @func: The #BtkTreeViewColumnFunc to use. 
 * @func_data: The user data for @func.
 * @destroy: The destroy notification for @func_data
 * 
 * Sets the #BtkTreeViewColumnFunc to use for the column.  This
 * function is used instead of the standard attributes mapping for
 * setting the column value, and should set the value of @tree_column's
 * cell renderer as appropriate.  @func may be %NULL to remove an
 * older one.
 **/
void
btk_tree_view_column_set_cell_data_func (BtkTreeViewColumn   *tree_column,
					 BtkCellRenderer     *cell_renderer,
					 BtkTreeCellDataFunc  func,
					 gpointer             func_data,
					 GDestroyNotify       destroy)
{
  btk_cell_layout_set_cell_data_func (BTK_CELL_LAYOUT (tree_column),
                                      cell_renderer,
                                      (BtkCellLayoutDataFunc)func,
                                      func_data, destroy);
}


/**
 * btk_tree_view_column_clear_attributes:
 * @tree_column: a #BtkTreeViewColumn
 * @cell_renderer: a #BtkCellRenderer to clear the attribute mapping on.
 * 
 * Clears all existing attributes previously set with
 * btk_tree_view_column_set_attributes().
 **/
void
btk_tree_view_column_clear_attributes (BtkTreeViewColumn *tree_column,
				       BtkCellRenderer   *cell_renderer)
{
  btk_cell_layout_clear_attributes (BTK_CELL_LAYOUT (tree_column),
                                    cell_renderer);
}

/**
 * btk_tree_view_column_set_spacing:
 * @tree_column: A #BtkTreeViewColumn.
 * @spacing: distance between cell renderers in pixels.
 * 
 * Sets the spacing field of @tree_column, which is the number of pixels to
 * place between cell renderers packed into it.
 **/
void
btk_tree_view_column_set_spacing (BtkTreeViewColumn *tree_column,
				  gint               spacing)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (spacing >= 0);

  if (tree_column->spacing == spacing)
    return;

  tree_column->spacing = spacing;
  if (tree_column->tree_view)
    _btk_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

/**
 * btk_tree_view_column_get_spacing:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the spacing of @tree_column.
 * 
 * Return value: the spacing of @tree_column.
 **/
gint
btk_tree_view_column_get_spacing (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->spacing;
}

/* Options for manipulating the columns */

/**
 * btk_tree_view_column_set_visible:
 * @tree_column: A #BtkTreeViewColumn.
 * @visible: %TRUE if the @tree_column is visible.
 * 
 * Sets the visibility of @tree_column.
 **/
void
btk_tree_view_column_set_visible (BtkTreeViewColumn *tree_column,
				  gboolean           visible)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  visible = !! visible;
  
  if (tree_column->visible == visible)
    return;

  tree_column->visible = visible;

  if (tree_column->visible)
    _btk_tree_view_column_cell_set_dirty (tree_column, TRUE);

  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "visible");
}

/**
 * btk_tree_view_column_get_visible:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns %TRUE if @tree_column is visible.
 * 
 * Return value: whether the column is visible or not.  If it is visible, then
 * the tree will show the column.
 **/
gboolean
btk_tree_view_column_get_visible (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->visible;
}

/**
 * btk_tree_view_column_set_resizable:
 * @tree_column: A #BtkTreeViewColumn
 * @resizable: %TRUE, if the column can be resized
 * 
 * If @resizable is %TRUE, then the user can explicitly resize the column by
 * grabbing the outer edge of the column button.  If resizable is %TRUE and
 * sizing mode of the column is #BTK_TREE_VIEW_COLUMN_AUTOSIZE, then the sizing
 * mode is changed to #BTK_TREE_VIEW_COLUMN_GROW_ONLY.
 **/
void
btk_tree_view_column_set_resizable (BtkTreeViewColumn *tree_column,
				    gboolean           resizable)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  resizable = !! resizable;

  if (tree_column->resizable == resizable)
    return;

  tree_column->resizable = resizable;

  if (resizable && tree_column->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
    btk_tree_view_column_set_sizing (tree_column, BTK_TREE_VIEW_COLUMN_GROW_ONLY);

  btk_tree_view_column_update_button (tree_column);

  g_object_notify (G_OBJECT (tree_column), "resizable");
}

/**
 * btk_tree_view_column_get_resizable:
 * @tree_column: A #BtkTreeViewColumn
 * 
 * Returns %TRUE if the @tree_column can be resized by the end user.
 * 
 * Return value: %TRUE, if the @tree_column can be resized.
 **/
gboolean
btk_tree_view_column_get_resizable (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->resizable;
}


/**
 * btk_tree_view_column_set_sizing:
 * @tree_column: A #BtkTreeViewColumn.
 * @type: The #BtkTreeViewColumnSizing.
 * 
 * Sets the growth behavior of @tree_column to @type.
 **/
void
btk_tree_view_column_set_sizing (BtkTreeViewColumn       *tree_column,
                                 BtkTreeViewColumnSizing  type)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (type == tree_column->column_type)
    return;

  if (type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
    btk_tree_view_column_set_resizable (tree_column, FALSE);

#if 0
  /* I was clearly on crack when I wrote this.  I'm not sure what's supposed to
   * be below so I'll leave it until I figure it out.
   */
  if (tree_column->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE &&
      tree_column->requested_width != -1)
    {
      btk_tree_view_column_set_sizing (tree_column, tree_column->requested_width);
    }
#endif
  tree_column->column_type = type;

  btk_tree_view_column_update_button (tree_column);

  g_object_notify (G_OBJECT (tree_column), "sizing");
}

/**
 * btk_tree_view_column_get_sizing:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the current type of @tree_column.
 * 
 * Return value: The type of @tree_column.
 **/
BtkTreeViewColumnSizing
btk_tree_view_column_get_sizing (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->column_type;
}

/**
 * btk_tree_view_column_get_width:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the current size of @tree_column in pixels.
 * 
 * Return value: The current width of @tree_column.
 **/
gint
btk_tree_view_column_get_width (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->width;
}

/**
 * btk_tree_view_column_set_fixed_width:
 * @tree_column: A #BtkTreeViewColumn.
 * @fixed_width: The size to set @tree_column to. Must be greater than 0.
 * 
 * Sets the size of the column in pixels.  This is meaningful only if the sizing
 * type is #BTK_TREE_VIEW_COLUMN_FIXED.  The size of the column is clamped to
 * the min/max width for the column.  Please note that the min/max width of the
 * column doesn't actually affect the "fixed_width" property of the widget, just
 * the actual size when displayed.
 **/
void
btk_tree_view_column_set_fixed_width (BtkTreeViewColumn *tree_column,
				      gint               fixed_width)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (fixed_width > 0);

  tree_column->fixed_width = fixed_width;
  tree_column->use_resized_width = FALSE;

  if (tree_column->tree_view &&
      btk_widget_get_realized (tree_column->tree_view) &&
      tree_column->column_type == BTK_TREE_VIEW_COLUMN_FIXED)
    {
      btk_widget_queue_resize (tree_column->tree_view);
    }

  g_object_notify (G_OBJECT (tree_column), "fixed-width");
}

/**
 * btk_tree_view_column_get_fixed_width:
 * @tree_column: a #BtkTreeViewColumn
 * 
 * Gets the fixed width of the column.  This value is only meaning may not be
 * the actual width of the column on the screen, just what is requested.
 * 
 * Return value: the fixed width of the column
 **/
gint
btk_tree_view_column_get_fixed_width (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->fixed_width;
}

/**
 * btk_tree_view_column_set_min_width:
 * @tree_column: A #BtkTreeViewColumn.
 * @min_width: The minimum width of the column in pixels, or -1.
 * 
 * Sets the minimum width of the @tree_column.  If @min_width is -1, then the
 * minimum width is unset.
 **/
void
btk_tree_view_column_set_min_width (BtkTreeViewColumn *tree_column,
				    gint               min_width)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (min_width >= -1);

  if (min_width == tree_column->min_width)
    return;

  if (tree_column->visible &&
      tree_column->tree_view != NULL &&
      btk_widget_get_realized (tree_column->tree_view))
    {
      if (min_width > tree_column->width)
	btk_widget_queue_resize (tree_column->tree_view);
    }

  tree_column->min_width = min_width;
  g_object_freeze_notify (G_OBJECT (tree_column));
  if (tree_column->max_width != -1 && tree_column->max_width < min_width)
    {
      tree_column->max_width = min_width;
      g_object_notify (G_OBJECT (tree_column), "max-width");
    }
  g_object_notify (G_OBJECT (tree_column), "min-width");
  g_object_thaw_notify (G_OBJECT (tree_column));

  if (tree_column->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
    _btk_tree_view_column_autosize (BTK_TREE_VIEW (tree_column->tree_view),
				    tree_column);
}

/**
 * btk_tree_view_column_get_min_width:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the minimum width in pixels of the @tree_column, or -1 if no minimum
 * width is set.
 * 
 * Return value: The minimum width of the @tree_column.
 **/
gint
btk_tree_view_column_get_min_width (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), -1);

  return tree_column->min_width;
}

/**
 * btk_tree_view_column_set_max_width:
 * @tree_column: A #BtkTreeViewColumn.
 * @max_width: The maximum width of the column in pixels, or -1.
 * 
 * Sets the maximum width of the @tree_column.  If @max_width is -1, then the
 * maximum width is unset.  Note, the column can actually be wider than max
 * width if it's the last column in a view.  In this case, the column expands to
 * fill any extra space.
 **/
void
btk_tree_view_column_set_max_width (BtkTreeViewColumn *tree_column,
				    gint               max_width)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (max_width >= -1);

  if (max_width == tree_column->max_width)
    return;

  if (tree_column->visible &&
      tree_column->tree_view != NULL &&
      btk_widget_get_realized (tree_column->tree_view))
    {
      if (max_width != -1 && max_width < tree_column->width)
	btk_widget_queue_resize (tree_column->tree_view);
    }

  tree_column->max_width = max_width;
  g_object_freeze_notify (G_OBJECT (tree_column));
  if (max_width != -1 && max_width < tree_column->min_width)
    {
      tree_column->min_width = max_width;
      g_object_notify (G_OBJECT (tree_column), "min-width");
    }
  g_object_notify (G_OBJECT (tree_column), "max-width");
  g_object_thaw_notify (G_OBJECT (tree_column));

  if (tree_column->column_type == BTK_TREE_VIEW_COLUMN_AUTOSIZE)
    _btk_tree_view_column_autosize (BTK_TREE_VIEW (tree_column->tree_view),
				    tree_column);
}

/**
 * btk_tree_view_column_get_max_width:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the maximum width in pixels of the @tree_column, or -1 if no maximum
 * width is set.
 * 
 * Return value: The maximum width of the @tree_column.
 **/
gint
btk_tree_view_column_get_max_width (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), -1);

  return tree_column->max_width;
}

/**
 * btk_tree_view_column_clicked:
 * @tree_column: a #BtkTreeViewColumn
 * 
 * Emits the "clicked" signal on the column.  This function will only work if
 * @tree_column is clickable.
 **/
void
btk_tree_view_column_clicked (BtkTreeViewColumn *tree_column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_column->visible &&
      tree_column->button &&
      tree_column->clickable)
    btk_button_clicked (BTK_BUTTON (tree_column->button));
}

/**
 * btk_tree_view_column_set_title:
 * @tree_column: A #BtkTreeViewColumn.
 * @title: The title of the @tree_column.
 * 
 * Sets the title of the @tree_column.  If a custom widget has been set, then
 * this value is ignored.
 **/
void
btk_tree_view_column_set_title (BtkTreeViewColumn *tree_column,
				const gchar       *title)
{
  gchar *new_title;
  
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  new_title = g_strdup (title);
  g_free (tree_column->title);
  tree_column->title = new_title;

  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "title");
}

/**
 * btk_tree_view_column_get_title:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the title of the widget.
 * 
 * Return value: the title of the column. This string should not be
 * modified or freed.
 **/
const gchar *
btk_tree_view_column_get_title (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->title;
}

/**
 * btk_tree_view_column_set_expand:
 * @tree_column: A #BtkTreeViewColumn
 * @expand: %TRUE if the column should take available extra space, %FALSE if not
 * 
 * Sets the column to take available extra space.  This space is shared equally
 * amongst all columns that have the expand set to %TRUE.  If no column has this
 * option set, then the last column gets all extra space.  By default, every
 * column is created with this %FALSE.
 *
 * Since: 2.4
 **/
void
btk_tree_view_column_set_expand (BtkTreeViewColumn *tree_column,
				 gboolean           expand)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  expand = expand?TRUE:FALSE;
  if (tree_column->expand == expand)
    return;
  tree_column->expand = expand;

  if (tree_column->visible &&
      tree_column->tree_view != NULL &&
      btk_widget_get_realized (tree_column->tree_view))
    {
      /* We want to continue using the original width of the
       * column that includes additional space added by the user
       * resizing the columns and possibly extra (expanded) space, which
       * are not included in the resized width.
       */
      tree_column->use_resized_width = FALSE;

      btk_widget_queue_resize (tree_column->tree_view);
    }

  g_object_notify (G_OBJECT (tree_column), "expand");
}

/**
 * btk_tree_view_column_get_expand:
 * @tree_column: a #BtkTreeViewColumn
 * 
 * Return %TRUE if the column expands to take any available space.
 * 
 * Return value: %TRUE, if the column expands
 *
 * Since: 2.4
 **/
gboolean
btk_tree_view_column_get_expand (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->expand;
}

/**
 * btk_tree_view_column_set_clickable:
 * @tree_column: A #BtkTreeViewColumn.
 * @clickable: %TRUE if the header is active.
 * 
 * Sets the header to be active if @active is %TRUE.  When the header is active,
 * then it can take keyboard focus, and can be clicked.
 **/
void
btk_tree_view_column_set_clickable (BtkTreeViewColumn *tree_column,
                                    gboolean           clickable)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  clickable = !! clickable;
  if (tree_column->clickable == clickable)
    return;

  tree_column->clickable = clickable;
  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "clickable");
}

/**
 * btk_tree_view_column_get_clickable:
 * @tree_column: a #BtkTreeViewColumn
 * 
 * Returns %TRUE if the user can click on the header for the column.
 * 
 * Return value: %TRUE if user can click the column header.
 **/
gboolean
btk_tree_view_column_get_clickable (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->clickable;
}

/**
 * btk_tree_view_column_set_widget:
 * @tree_column: A #BtkTreeViewColumn.
 * @widget: (allow-none): A child #BtkWidget, or %NULL.
 *
 * Sets the widget in the header to be @widget.  If widget is %NULL, then the
 * header button is set with a #BtkLabel set to the title of @tree_column.
 **/
void
btk_tree_view_column_set_widget (BtkTreeViewColumn *tree_column,
				 BtkWidget         *widget)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (widget == NULL || BTK_IS_WIDGET (widget));

  if (widget)
    g_object_ref_sink (widget);

  if (tree_column->child)      
    g_object_unref (tree_column->child);

  tree_column->child = widget;
  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "widget");
}

/**
 * btk_tree_view_column_get_widget:
 * @tree_column: A #BtkTreeViewColumn.
 *
 * Returns the #BtkWidget in the button on the column header.
 * If a custom widget has not been set then %NULL is returned.
 *
 * Return value: (transfer none): The #BtkWidget in the column
 *     header, or %NULL
 **/
BtkWidget *
btk_tree_view_column_get_widget (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->child;
}

/**
 * btk_tree_view_column_set_alignment:
 * @tree_column: A #BtkTreeViewColumn.
 * @xalign: The alignment, which is between [0.0 and 1.0] inclusive.
 * 
 * Sets the alignment of the title or custom widget inside the column header.
 * The alignment determines its location inside the button -- 0.0 for left, 0.5
 * for center, 1.0 for right.
 **/
void
btk_tree_view_column_set_alignment (BtkTreeViewColumn *tree_column,
                                    gfloat             xalign)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  xalign = CLAMP (xalign, 0.0, 1.0);

  if (tree_column->xalign == xalign)
    return;

  tree_column->xalign = xalign;
  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "alignment");
}

/**
 * btk_tree_view_column_get_alignment:
 * @tree_column: A #BtkTreeViewColumn.
 * 
 * Returns the current x alignment of @tree_column.  This value can range
 * between 0.0 and 1.0.
 * 
 * Return value: The current alignent of @tree_column.
 **/
gfloat
btk_tree_view_column_get_alignment (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0.5);

  return tree_column->xalign;
}

/**
 * btk_tree_view_column_set_reorderable:
 * @tree_column: A #BtkTreeViewColumn
 * @reorderable: %TRUE, if the column can be reordered.
 * 
 * If @reorderable is %TRUE, then the column can be reordered by the end user
 * dragging the header.
 **/
void
btk_tree_view_column_set_reorderable (BtkTreeViewColumn *tree_column,
				      gboolean           reorderable)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  /*  if (reorderable)
      btk_tree_view_column_set_clickable (tree_column, TRUE);*/

  if (tree_column->reorderable == (reorderable?TRUE:FALSE))
    return;

  tree_column->reorderable = (reorderable?TRUE:FALSE);
  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "reorderable");
}

/**
 * btk_tree_view_column_get_reorderable:
 * @tree_column: A #BtkTreeViewColumn
 * 
 * Returns %TRUE if the @tree_column can be reordered by the user.
 * 
 * Return value: %TRUE if the @tree_column can be reordered by the user.
 **/
gboolean
btk_tree_view_column_get_reorderable (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->reorderable;
}


/**
 * btk_tree_view_column_set_sort_column_id:
 * @tree_column: a #BtkTreeViewColumn
 * @sort_column_id: The @sort_column_id of the model to sort on.
 *
 * Sets the logical @sort_column_id that this column sorts on when this column 
 * is selected for sorting.  Doing so makes the column header clickable.
 **/
void
btk_tree_view_column_set_sort_column_id (BtkTreeViewColumn *tree_column,
					 gint               sort_column_id)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (sort_column_id >= -1);

  if (tree_column->sort_column_id == sort_column_id)
    return;

  tree_column->sort_column_id = sort_column_id;

  /* Handle unsetting the id */
  if (sort_column_id == -1)
    {
      BtkTreeModel *model = btk_tree_view_get_model (BTK_TREE_VIEW (tree_column->tree_view));

      if (tree_column->sort_clicked_signal)
	{
	  g_signal_handler_disconnect (tree_column, tree_column->sort_clicked_signal);
	  tree_column->sort_clicked_signal = 0;
	}

      if (tree_column->sort_column_changed_signal)
	{
	  g_signal_handler_disconnect (model, tree_column->sort_column_changed_signal);
	  tree_column->sort_column_changed_signal = 0;
	}

      btk_tree_view_column_set_sort_order (tree_column, BTK_SORT_ASCENDING);
      btk_tree_view_column_set_sort_indicator (tree_column, FALSE);
      btk_tree_view_column_set_clickable (tree_column, FALSE);
      g_object_notify (G_OBJECT (tree_column), "sort-column-id");
      return;
    }

  btk_tree_view_column_set_clickable (tree_column, TRUE);

  if (! tree_column->sort_clicked_signal)
    tree_column->sort_clicked_signal = g_signal_connect (tree_column,
                                                         "clicked",
                                                         G_CALLBACK (btk_tree_view_column_sort),
                                                         NULL);

  btk_tree_view_column_setup_sort_column_id_callback (tree_column);
  g_object_notify (G_OBJECT (tree_column), "sort-column-id");
}

/**
 * btk_tree_view_column_get_sort_column_id:
 * @tree_column: a #BtkTreeViewColumn
 *
 * Gets the logical @sort_column_id that the model sorts on when this
 * column is selected for sorting.
 * See btk_tree_view_column_set_sort_column_id().
 *
 * Return value: the current @sort_column_id for this column, or -1 if
 *               this column can't be used for sorting.
 **/
gint
btk_tree_view_column_get_sort_column_id (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->sort_column_id;
}

/**
 * btk_tree_view_column_set_sort_indicator:
 * @tree_column: a #BtkTreeViewColumn
 * @setting: %TRUE to display an indicator that the column is sorted
 *
 * Call this function with a @setting of %TRUE to display an arrow in
 * the header button indicating the column is sorted. Call
 * btk_tree_view_column_set_sort_order() to change the direction of
 * the arrow.
 * 
 **/
void
btk_tree_view_column_set_sort_indicator (BtkTreeViewColumn     *tree_column,
                                         gboolean               setting)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  setting = setting != FALSE;

  if (setting == tree_column->show_sort_indicator)
    return;

  tree_column->show_sort_indicator = setting;
  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "sort-indicator");
}

/**
 * btk_tree_view_column_get_sort_indicator:
 * @tree_column: a #BtkTreeViewColumn
 * 
 * Gets the value set by btk_tree_view_column_set_sort_indicator().
 * 
 * Return value: whether the sort indicator arrow is displayed
 **/
gboolean
btk_tree_view_column_get_sort_indicator  (BtkTreeViewColumn     *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return tree_column->show_sort_indicator;
}

/**
 * btk_tree_view_column_set_sort_order:
 * @tree_column: a #BtkTreeViewColumn
 * @order: sort order that the sort indicator should indicate
 *
 * Changes the appearance of the sort indicator. 
 * 
 * This <emphasis>does not</emphasis> actually sort the model.  Use
 * btk_tree_view_column_set_sort_column_id() if you want automatic sorting
 * support.  This function is primarily for custom sorting behavior, and should
 * be used in conjunction with btk_tree_sortable_set_sort_column() to do
 * that. For custom models, the mechanism will vary. 
 * 
 * The sort indicator changes direction to indicate normal sort or reverse sort.
 * Note that you must have the sort indicator enabled to see anything when 
 * calling this function; see btk_tree_view_column_set_sort_indicator().
 **/
void
btk_tree_view_column_set_sort_order      (BtkTreeViewColumn     *tree_column,
                                          BtkSortType            order)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (order == tree_column->sort_order)
    return;

  tree_column->sort_order = order;
  btk_tree_view_column_update_button (tree_column);
  g_object_notify (G_OBJECT (tree_column), "sort-order");
}

/**
 * btk_tree_view_column_get_sort_order:
 * @tree_column: a #BtkTreeViewColumn
 * 
 * Gets the value set by btk_tree_view_column_set_sort_order().
 * 
 * Return value: the sort order the sort indicator is indicating
 **/
BtkSortType
btk_tree_view_column_get_sort_order      (BtkTreeViewColumn     *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), 0);

  return tree_column->sort_order;
}

/**
 * btk_tree_view_column_cell_set_cell_data:
 * @tree_column: A #BtkTreeViewColumn.
 * @tree_model: The #BtkTreeModel to to get the cell renderers attributes from.
 * @iter: The #BtkTreeIter to to get the cell renderer's attributes from.
 * @is_expander: %TRUE, if the row has children
 * @is_expanded: %TRUE, if the row has visible children
 * 
 * Sets the cell renderer based on the @tree_model and @iter.  That is, for
 * every attribute mapping in @tree_column, it will get a value from the set
 * column on the @iter, and use that value to set the attribute on the cell
 * renderer.  This is used primarily by the #BtkTreeView.
 **/
void
btk_tree_view_column_cell_set_cell_data (BtkTreeViewColumn *tree_column,
					 BtkTreeModel      *tree_model,
					 BtkTreeIter       *iter,
					 gboolean           is_expander,
					 gboolean           is_expanded)
{
  GSList *list;
  GValue value = { 0, };
  GList *cell_list;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_model == NULL)
    return;

  for (cell_list = tree_column->cell_list; cell_list; cell_list = cell_list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) cell_list->data;
      GObject *cell = (GObject *) info->cell;

      list = info->attributes;

      g_object_freeze_notify (cell);

      if (info->cell->is_expander != is_expander)
	g_object_set (cell, "is-expander", is_expander, NULL);

      if (info->cell->is_expanded != is_expanded)
	g_object_set (cell, "is-expanded", is_expanded, NULL);

      while (list && list->next)
	{
	  btk_tree_model_get_value (tree_model, iter,
				    GPOINTER_TO_INT (list->next->data),
				    &value);
	  g_object_set_property (cell, (gchar *) list->data, &value);
	  g_value_unset (&value);
	  list = list->next->next;
	}

      if (info->func)
	(* info->func) (tree_column, info->cell, tree_model, iter, info->func_data);
      g_object_thaw_notify (G_OBJECT (info->cell));
    }

}

/**
 * btk_tree_view_column_cell_get_size:
 * @tree_column: A #BtkTreeViewColumn.
 * @cell_area: (allow-none): The area a cell in the column will be allocated, or %NULL
 * @x_offset: (out) (allow-none): location to return x offset of a cell relative to @cell_area, or %NULL
 * @y_offset: (out) (allow-none): location to return y offset of a cell relative to @cell_area, or %NULL
 * @width: (out) (allow-none): location to return width needed to render a cell, or %NULL
 * @height: (out) (allow-none): location to return height needed to render a cell, or %NULL
 * 
 * Obtains the width and height needed to render the column.  This is used
 * primarily by the #BtkTreeView.
 **/
void
btk_tree_view_column_cell_get_size (BtkTreeViewColumn  *tree_column,
				    const BdkRectangle *cell_area,
				    gint               *x_offset,
				    gint               *y_offset,
				    gint               *width,
				    gint               *height)
{
  GList *list;
  gboolean first_cell = TRUE;
  gint focus_line_width;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (height)
    * height = 0;
  if (width)
    * width = 0;

  btk_widget_style_get (tree_column->tree_view, "focus-line-width", &focus_line_width, NULL);
  
  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) list->data;
      gboolean visible;
      gint new_height = 0;
      gint new_width = 0;
      g_object_get (info->cell, "visible", &visible, NULL);

      if (visible == FALSE)
	continue;

      if (first_cell == FALSE && width)
	*width += tree_column->spacing;

      btk_cell_renderer_get_size (info->cell,
				  tree_column->tree_view,
				  cell_area,
				  x_offset,
				  y_offset,
				  &new_width,
				  &new_height);

      if (height)
	* height = MAX (*height, new_height + focus_line_width * 2);
      info->requested_width = MAX (info->requested_width, new_width + focus_line_width * 2);
      if (width)
	* width += info->requested_width;
      first_cell = FALSE;
    }
}

/* rendering, event handling and rendering focus are somewhat complicated, and
 * quite a bit of code.  Rather than duplicate them, we put them together to
 * keep the code in one place.
 *
 * To better understand what's going on, check out
 * docs/tree-column-sizing.png
 */
enum {
  CELL_ACTION_RENDER,
  CELL_ACTION_FOCUS,
  CELL_ACTION_EVENT
};

static gboolean
btk_tree_view_column_cell_process_action (BtkTreeViewColumn  *tree_column,
					  BdkWindow          *window,
					  const BdkRectangle *background_area,
					  const BdkRectangle *cell_area,
					  guint               flags,
					  gint                action,
					  const BdkRectangle *expose_area,     /* RENDER */
					  BdkRectangle       *focus_rectangle, /* FOCUS  */
					  BtkCellEditable   **editable_widget, /* EVENT  */
					  BdkEvent           *event,           /* EVENT  */
					  gchar              *path_string)     /* EVENT  */
{
  GList *list;
  BdkRectangle real_cell_area;
  BdkRectangle real_background_area;
  BdkRectangle real_expose_area = *cell_area;
  gint depth = 0;
  gint expand_cell_count = 0;
  gint full_requested_width = 0;
  gint extra_space;
  gint min_x, min_y, max_x, max_y;
  gint focus_line_width;
  gint special_cells;
  gint horizontal_separator;
  gboolean cursor_row = FALSE;
  gboolean first_cell = TRUE;
  gboolean rtl;
  /* If we have rtl text, we need to transform our areas */
  BdkRectangle rtl_cell_area;
  BdkRectangle rtl_background_area;

  min_x = G_MAXINT;
  min_y = G_MAXINT;
  max_x = 0;
  max_y = 0;

  rtl = (btk_widget_get_direction (BTK_WIDGET (tree_column->tree_view)) == BTK_TEXT_DIR_RTL);
  special_cells = _btk_tree_view_column_count_special_cells (tree_column);

  if (special_cells > 1 && action == CELL_ACTION_FOCUS)
    {
      BtkTreeViewColumnCellInfo *info = NULL;
      gboolean found_has_focus = FALSE;

      /* one should have focus */
      for (list = tree_column->cell_list; list; list = list->next)
        {
	  info = list->data;
	  if (info && info->has_focus)
	    {
	      found_has_focus = TRUE;
	      break;
	    }
	}

      if (!found_has_focus)
        {
	  /* give the first one focus */
	  info = btk_tree_view_column_cell_first (tree_column)->data;
	  info->has_focus = TRUE;
	}
    }

  cursor_row = flags & BTK_CELL_RENDERER_FOCUSED;

  btk_widget_style_get (BTK_WIDGET (tree_column->tree_view),
			"focus-line-width", &focus_line_width,
			"horizontal-separator", &horizontal_separator,
			NULL);

  real_cell_area = *cell_area;
  real_background_area = *background_area;


  real_cell_area.x += focus_line_width;
  real_cell_area.y += focus_line_width;
  real_cell_area.height -= 2 * focus_line_width;

  if (rtl)
    depth = real_background_area.width - real_cell_area.width;
  else
    depth = real_cell_area.x - real_background_area.x;

  /* Find out how much extra space we have to allocate */
  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *)list->data;

      if (! info->cell->visible)
	continue;

      if (info->expand == TRUE)
	expand_cell_count ++;
      full_requested_width += info->requested_width;

      if (!first_cell)
	full_requested_width += tree_column->spacing;

      first_cell = FALSE;
    }

  extra_space = cell_area->width - full_requested_width;
  if (extra_space < 0)
    extra_space = 0;
  else if (extra_space > 0 && expand_cell_count > 0)
    extra_space /= expand_cell_count;

  /* iterate list for BTK_PACK_START cells */
  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) list->data;

      if (info->pack == BTK_PACK_END)
	continue;

      if (! info->cell->visible)
	continue;

      if ((info->has_focus || special_cells == 1) && cursor_row)
	flags |= BTK_CELL_RENDERER_FOCUSED;
      else
        flags &= ~BTK_CELL_RENDERER_FOCUSED;

      info->real_width = info->requested_width + (info->expand?extra_space:0);

      /* We constrain ourselves to only the width available */
      if (real_cell_area.x - focus_line_width + info->real_width > cell_area->x + cell_area->width)
	{
	  info->real_width = cell_area->x + cell_area->width - real_cell_area.x;
	}   

      if (real_cell_area.x > cell_area->x + cell_area->width)
	break;

      real_cell_area.width = info->real_width;
      real_cell_area.width -= 2 * focus_line_width;

      if (list->next)
	{
	  real_background_area.width = info->real_width + depth;
	}
      else
	{
          /* fill the rest of background for the last cell */
	  real_background_area.width = background_area->x + background_area->width - real_background_area.x;
	}

      rtl_cell_area = real_cell_area;
      rtl_background_area = real_background_area;
      
      if (rtl)
	{
	  rtl_cell_area.x = cell_area->x + cell_area->width - (real_cell_area.x - cell_area->x) - real_cell_area.width;
	  rtl_background_area.x = background_area->x + background_area->width - (real_background_area.x - background_area->x) - real_background_area.width;
	}

      /* RENDER */
      if (action == CELL_ACTION_RENDER)
	{
	  btk_cell_renderer_render (info->cell,
				    window,
				    tree_column->tree_view,
				    &rtl_background_area,
				    &rtl_cell_area,
				    &real_expose_area, 
				    flags);
	}
      /* FOCUS */
      else if (action == CELL_ACTION_FOCUS)
	{
	  gint x_offset, y_offset, width, height;

	  btk_cell_renderer_get_size (info->cell,
				      tree_column->tree_view,
				      &rtl_cell_area,
				      &x_offset, &y_offset,
				      &width, &height);

	  if (special_cells > 1)
	    {
	      if (info->has_focus)
	        {
		  min_x = rtl_cell_area.x + x_offset;
		  max_x = min_x + width;
		  min_y = rtl_cell_area.y + y_offset;
		  max_y = min_y + height;
		}
	    }
	  else
	    {
	      if (min_x > (rtl_cell_area.x + x_offset))
		min_x = rtl_cell_area.x + x_offset;
	      if (max_x < rtl_cell_area.x + x_offset + width)
		max_x = rtl_cell_area.x + x_offset + width;
	      if (min_y > (rtl_cell_area.y + y_offset))
		min_y = rtl_cell_area.y + y_offset;
	      if (max_y < rtl_cell_area.y + y_offset + height)
		max_y = rtl_cell_area.y + y_offset + height;
	    }
	}
      /* EVENT */
      else if (action == CELL_ACTION_EVENT)
	{
	  gboolean try_event = FALSE;

	  if (event)
	    {
	      if (special_cells == 1)
	        {
		  /* only 1 activatable cell -> whole column can activate */
		  if (cell_area->x <= ((BdkEventButton *)event)->x &&
		      cell_area->x + cell_area->width > ((BdkEventButton *)event)->x)
		    try_event = TRUE;
		}
	      else if (rtl_cell_area.x <= ((BdkEventButton *)event)->x &&
		  rtl_cell_area.x + rtl_cell_area.width > ((BdkEventButton *)event)->x)
		  /* only activate cell if the user clicked on an individual
		   * cell
		   */
		try_event = TRUE;
	    }
	  else if (special_cells > 1 && info->has_focus)
	    try_event = TRUE;
	  else if (special_cells == 1)
	    try_event = TRUE;

	  if (try_event)
	    {
	      gboolean visible, mode;

	      g_object_get (info->cell,
			    "visible", &visible,
			    "mode", &mode,
			    NULL);
	      if (visible && mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE)
		{
		  if (btk_cell_renderer_activate (info->cell,
						  event,
						  tree_column->tree_view,
						  path_string,
						  &rtl_background_area,
						  &rtl_cell_area,
						  flags))
		    {
                      flags &= ~BTK_CELL_RENDERER_FOCUSED;
		      return TRUE;
		    }
		}
	      else if (visible && mode == BTK_CELL_RENDERER_MODE_EDITABLE)
		{
		  *editable_widget =
		    btk_cell_renderer_start_editing (info->cell,
						     event,
						     tree_column->tree_view,
						     path_string,
						     &rtl_background_area,
						     &rtl_cell_area,
						     flags);

		  if (*editable_widget != NULL)
		    {
		      g_return_val_if_fail (BTK_IS_CELL_EDITABLE (*editable_widget), FALSE);
		      info->in_editing_mode = TRUE;
		      btk_tree_view_column_focus_cell (tree_column, info->cell);
		      
                      flags &= ~BTK_CELL_RENDERER_FOCUSED;

		      return TRUE;
		    }
		}
	    }
	}

      flags &= ~BTK_CELL_RENDERER_FOCUSED;

      real_cell_area.x += (real_cell_area.width + 2 * focus_line_width + tree_column->spacing);
      real_background_area.x += real_background_area.width + tree_column->spacing;

      /* Only needed for first cell */
      depth = 0;
    }

  /* iterate list for PACK_END cells */
  for (list = g_list_last (tree_column->cell_list); list; list = list->prev)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) list->data;

      if (info->pack == BTK_PACK_START)
	continue;

      if (! info->cell->visible)
	continue;

      if ((info->has_focus || special_cells == 1) && cursor_row)
	flags |= BTK_CELL_RENDERER_FOCUSED;
      else
        flags &= ~BTK_CELL_RENDERER_FOCUSED;

      info->real_width = info->requested_width + (info->expand?extra_space:0);

      /* We constrain ourselves to only the width available */
      if (real_cell_area.x - focus_line_width + info->real_width > cell_area->x + cell_area->width)
	{
	  info->real_width = cell_area->x + cell_area->width - real_cell_area.x;
	}   

      if (real_cell_area.x > cell_area->x + cell_area->width)
	break;

      real_cell_area.width = info->real_width;
      real_cell_area.width -= 2 * focus_line_width;
      real_background_area.width = info->real_width + depth;

      rtl_cell_area = real_cell_area;
      rtl_background_area = real_background_area;
      if (rtl)
	{
	  rtl_cell_area.x = cell_area->x + cell_area->width - (real_cell_area.x - cell_area->x) - real_cell_area.width;
	  rtl_background_area.x = background_area->x + background_area->width - (real_background_area.x - background_area->x) - real_background_area.width;
	}

      /* RENDER */
      if (action == CELL_ACTION_RENDER)
	{
	  btk_cell_renderer_render (info->cell,
				    window,
				    tree_column->tree_view,
				    &rtl_background_area,
				    &rtl_cell_area,
				    &real_expose_area,
				    flags);
	}
      /* FOCUS */
      else if (action == CELL_ACTION_FOCUS)
	{
	  gint x_offset, y_offset, width, height;

	  btk_cell_renderer_get_size (info->cell,
				      tree_column->tree_view,
				      &rtl_cell_area,
				      &x_offset, &y_offset,
				      &width, &height);

	  if (special_cells > 1)
	    {
	      if (info->has_focus)
	        {
		  min_x = rtl_cell_area.x + x_offset;
		  max_x = min_x + width;
		  min_y = rtl_cell_area.y + y_offset;
		  max_y = min_y + height;
		}
	    }
	  else
	    {
	      if (min_x > (rtl_cell_area.x + x_offset))
		min_x = rtl_cell_area.x + x_offset;
	      if (max_x < rtl_cell_area.x + x_offset + width)
		max_x = rtl_cell_area.x + x_offset + width;
	      if (min_y > (rtl_cell_area.y + y_offset))
		min_y = rtl_cell_area.y + y_offset;
	      if (max_y < rtl_cell_area.y + y_offset + height)
		max_y = rtl_cell_area.y + y_offset + height;
	    }
	}
      /* EVENT */
      else if (action == CELL_ACTION_EVENT)
        {
	  gboolean try_event = FALSE;

	  if (event)
	    {
	      if (special_cells == 1)
	        {
		  /* only 1 activatable cell -> whole column can activate */
		  if (cell_area->x <= ((BdkEventButton *)event)->x &&
		      cell_area->x + cell_area->width > ((BdkEventButton *)event)->x)
		    try_event = TRUE;
		}
	      else if (rtl_cell_area.x <= ((BdkEventButton *)event)->x &&
		  rtl_cell_area.x + rtl_cell_area.width > ((BdkEventButton *)event)->x)
		/* only activate cell if the user clicked on an individual
		 * cell
		 */
		try_event = TRUE;
	    }
	  else if (special_cells > 1 && info->has_focus)
	    try_event = TRUE;
	  else if (special_cells == 1)
	    try_event = TRUE;

	  if (try_event)
	    {
	      gboolean visible, mode;

	      g_object_get (info->cell,
			    "visible", &visible,
			    "mode", &mode,
			    NULL);
	      if (visible && mode == BTK_CELL_RENDERER_MODE_ACTIVATABLE)
	        {
		  if (btk_cell_renderer_activate (info->cell,
						  event,
						  tree_column->tree_view,
						  path_string,
						  &rtl_background_area,
						  &rtl_cell_area,
						  flags))
		    {
		      flags &= ~BTK_CELL_RENDERER_FOCUSED;
		      return TRUE;
		    }
		}
	      else if (visible && mode == BTK_CELL_RENDERER_MODE_EDITABLE)
	        {
		  *editable_widget =
		    btk_cell_renderer_start_editing (info->cell,
						     event,
						     tree_column->tree_view,
						     path_string,
						     &rtl_background_area,
						     &rtl_cell_area,
						     flags);

		  if (*editable_widget != NULL)
		    {
		      g_return_val_if_fail (BTK_IS_CELL_EDITABLE (*editable_widget), FALSE);
		      info->in_editing_mode = TRUE;
		      btk_tree_view_column_focus_cell (tree_column, info->cell);

		      flags &= ~BTK_CELL_RENDERER_FOCUSED;
		      return TRUE;
		    }
		}
	    }
	}

      flags &= ~BTK_CELL_RENDERER_FOCUSED;

      real_cell_area.x += (real_cell_area.width + 2 * focus_line_width + tree_column->spacing);
      real_background_area.x += (real_background_area.width + tree_column->spacing);

      /* Only needed for first cell */
      depth = 0;
    }

  /* fill focus_rectangle when required */
  if (action == CELL_ACTION_FOCUS)
    {
      if (min_x >= max_x || min_y >= max_y)
	{
	  *focus_rectangle = *cell_area;
	  /* don't change the focus_rectangle, just draw it nicely inside
	   * the cell area */
	}
      else
	{
	  focus_rectangle->x = min_x - focus_line_width;
	  focus_rectangle->y = min_y - focus_line_width;
	  focus_rectangle->width = (max_x - min_x) + 2 * focus_line_width;
	  focus_rectangle->height = (max_y - min_y) + 2 * focus_line_width;
	}
    }

  return FALSE;
}

/**
 * btk_tree_view_column_cell_render:
 * @tree_column: A #BtkTreeViewColumn.
 * @window: a #BdkDrawable to draw to
 * @background_area: entire cell area (including tree expanders and maybe padding on the sides)
 * @cell_area: area normally rendered by a cell renderer
 * @expose_area: area that actually needs updating
 * @flags: flags that affect rendering
 * 
 * Renders the cell contained by #tree_column. This is used primarily by the
 * #BtkTreeView.
 **/
void
_btk_tree_view_column_cell_render (BtkTreeViewColumn  *tree_column,
				   BdkWindow          *window,
				   const BdkRectangle *background_area,
				   const BdkRectangle *cell_area,
				   const BdkRectangle *expose_area,
				   guint               flags)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (background_area != NULL);
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (expose_area != NULL);

  btk_tree_view_column_cell_process_action (tree_column,
					    window,
					    background_area,
					    cell_area,
					    flags,
					    CELL_ACTION_RENDER,
					    expose_area,
					    NULL, NULL, NULL, NULL);
}

gboolean
_btk_tree_view_column_cell_event (BtkTreeViewColumn  *tree_column,
				  BtkCellEditable   **editable_widget,
				  BdkEvent           *event,
				  gchar              *path_string,
				  const BdkRectangle *background_area,
				  const BdkRectangle *cell_area,
				  guint               flags)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  return btk_tree_view_column_cell_process_action (tree_column,
						   NULL,
						   background_area,
						   cell_area,
						   flags,
						   CELL_ACTION_EVENT,
						   NULL, NULL,
						   editable_widget,
						   event,
						   path_string);
}

void
_btk_tree_view_column_get_focus_area (BtkTreeViewColumn  *tree_column,
				      const BdkRectangle *background_area,
				      const BdkRectangle *cell_area,
				      BdkRectangle       *focus_area)
{
  btk_tree_view_column_cell_process_action (tree_column,
					    NULL,
					    background_area,
					    cell_area,
					    0,
					    CELL_ACTION_FOCUS,
					    NULL,
					    focus_area,
					    NULL, NULL, NULL);
}


/* cell list manipulation */
static GList *
btk_tree_view_column_cell_first (BtkTreeViewColumn *tree_column)
{
  GList *list = tree_column->cell_list;

  /* first BTK_PACK_START cell we find */
  for ( ; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = list->data;
      if (info->pack == BTK_PACK_START)
	return list;
    }

  /* hmm, else the *last* BTK_PACK_END cell */
  list = g_list_last (tree_column->cell_list);

  for ( ; list; list = list->prev)
    {
      BtkTreeViewColumnCellInfo *info = list->data;
      if (info->pack == BTK_PACK_END)
	return list;
    }

  return NULL;
}

static GList *
btk_tree_view_column_cell_last (BtkTreeViewColumn *tree_column)
{
  GList *list = tree_column->cell_list;

  /* *first* BTK_PACK_END cell we find */
  for ( ; list ; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = list->data;
      if (info->pack == BTK_PACK_END)
	return list;
    }

  /* hmm, else the last BTK_PACK_START cell */
  list = g_list_last (tree_column->cell_list);

  for ( ; list; list = list->prev)
    {
      BtkTreeViewColumnCellInfo *info = list->data;
      if (info->pack == BTK_PACK_START)
	return list;
    }

  return NULL;
}

static GList *
btk_tree_view_column_cell_next (BtkTreeViewColumn *tree_column,
				GList             *current)
{
  GList *list;
  BtkTreeViewColumnCellInfo *info = current->data;

  if (info->pack == BTK_PACK_START)
    {
      for (list = current->next; list; list = list->next)
        {
	  BtkTreeViewColumnCellInfo *inf = list->data;
	  if (inf->pack == BTK_PACK_START)
	    return list;
	}

      /* out of BTK_PACK_START cells, get *last* BTK_PACK_END one */
      list = g_list_last (tree_column->cell_list);
      for (; list; list = list->prev)
        {
	  BtkTreeViewColumnCellInfo *inf = list->data;
	  if (inf->pack == BTK_PACK_END)
	    return list;
	}
    }

  for (list = current->prev; list; list = list->prev)
    {
      BtkTreeViewColumnCellInfo *inf = list->data;
      if (inf->pack == BTK_PACK_END)
	return list;
    }

  return NULL;
}

static GList *
btk_tree_view_column_cell_prev (BtkTreeViewColumn *tree_column,
				GList             *current)
{
  GList *list;
  BtkTreeViewColumnCellInfo *info = current->data;

  if (info->pack == BTK_PACK_END)
    {
      for (list = current->next; list; list = list->next)
        {
	  BtkTreeViewColumnCellInfo *inf = list->data;
	  if (inf->pack == BTK_PACK_END)
	    return list;
	}

      /* out of BTK_PACK_END, get last BTK_PACK_START one */
      list = g_list_last (tree_column->cell_list);
      for ( ; list; list = list->prev)
        {
	  BtkTreeViewColumnCellInfo *inf = list->data;
	  if (inf->pack == BTK_PACK_START)
	    return list;
	}
    }

  for (list = current->prev; list; list = list->prev)
    {
      BtkTreeViewColumnCellInfo *inf = list->data;
      if (inf->pack == BTK_PACK_START)
	return list;
    }

  return NULL;
}

gboolean
_btk_tree_view_column_cell_focus (BtkTreeViewColumn *tree_column,
				  gint               direction,
				  gboolean           left,
				  gboolean           right)
{
  gint count;
  gboolean rtl;

  count = _btk_tree_view_column_count_special_cells (tree_column);
  rtl = btk_widget_get_direction (BTK_WIDGET (tree_column->tree_view)) == BTK_TEXT_DIR_RTL;

  /* if we are the current focus column and have multiple editable cells,
   * try to select the next one, else move the focus to the next column
   */
  if (BTK_TREE_VIEW (tree_column->tree_view)->priv->focus_column == tree_column)
    {
      if (count > 1)
        {
          GList *next, *prev;
	  GList *list = tree_column->cell_list;
	  BtkTreeViewColumnCellInfo *info = NULL;

	  /* find current focussed cell */
	  for ( ; list; list = list->next)
	    {
	      info = list->data;
	      if (info->has_focus)
		break;
	    }

	  /* not a focussed cell in the focus column? */
	  if (!list || !info || !info->has_focus)
	    return FALSE;

	  if (rtl)
	    {
	      prev = btk_tree_view_column_cell_next (tree_column, list);
	      next = btk_tree_view_column_cell_prev (tree_column, list);
	    }
	  else
	    {
	      next = btk_tree_view_column_cell_next (tree_column, list);
	      prev = btk_tree_view_column_cell_prev (tree_column, list);
	    }

	  info->has_focus = FALSE;
	  if (direction > 0 && next)
	    {
	      info = next->data;
	      info->has_focus = TRUE;
	      return TRUE;
	    }
	  else if (direction > 0 && !next && !right)
	    {
	      /* keep focus on last cell */
	      if (rtl)
	        info = btk_tree_view_column_cell_first (tree_column)->data;
	      else
	        info = btk_tree_view_column_cell_last (tree_column)->data;

	      info->has_focus = TRUE;
	      return TRUE;
	    }
	  else if (direction < 0 && prev)
	    {
	      info = prev->data;
	      info->has_focus = TRUE;
	      return TRUE;
	    }
	  else if (direction < 0 && !prev && !left)
	    {
	      /* keep focus on first cell */
	      if (rtl)
		info = btk_tree_view_column_cell_last (tree_column)->data;
	      else
		info = btk_tree_view_column_cell_first (tree_column)->data;

	      info->has_focus = TRUE;
	      return TRUE;
	    }
	}
      return FALSE;
    }

  /* we get focus, if we have multiple editable cells, give the correct one
   * focus
   */
  if (count > 1)
    {
      GList *list = tree_column->cell_list;

      /* clear focus first */
      for ( ; list ; list = list->next)
        {
	  BtkTreeViewColumnCellInfo *info = list->data;
	  if (info->has_focus)
	    info->has_focus = FALSE;
	}

      list = NULL;
      if (rtl)
        {
	  if (direction > 0)
	    list = btk_tree_view_column_cell_last (tree_column);
	  else if (direction < 0)
	    list = btk_tree_view_column_cell_first (tree_column);
	}
      else
        {
	  if (direction > 0)
	    list = btk_tree_view_column_cell_first (tree_column);
	  else if (direction < 0)
	    list = btk_tree_view_column_cell_last (tree_column);
	}

      if (list)
	((BtkTreeViewColumnCellInfo *) list->data)->has_focus = TRUE;
    }

  return TRUE;
}

void
_btk_tree_view_column_cell_draw_focus (BtkTreeViewColumn  *tree_column,
				       BdkWindow          *window,
				       const BdkRectangle *background_area,
				       const BdkRectangle *cell_area,
				       const BdkRectangle *expose_area,
				       guint               flags)
{
  gint focus_line_width;
  BtkStateType cell_state;
  
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  btk_widget_style_get (BTK_WIDGET (tree_column->tree_view),
			"focus-line-width", &focus_line_width, NULL);
  if (tree_column->editable_widget)
    {
      /* This function is only called on the editable row when editing.
       */
#if 0
      btk_paint_focus (tree_column->tree_view->style,
		       window,
		       btk_widget_get_state (tree_column->tree_view),
		       NULL,
		       tree_column->tree_view,
		       "treeview",
		       cell_area->x - focus_line_width,
		       cell_area->y - focus_line_width,
		       cell_area->width + 2 * focus_line_width,
		       cell_area->height + 2 * focus_line_width);
#endif      
    }
  else
    {
      BdkRectangle focus_rectangle;
      btk_tree_view_column_cell_process_action (tree_column,
						window,
						background_area,
						cell_area,
						flags,
						CELL_ACTION_FOCUS,
						expose_area,
						&focus_rectangle,
						NULL, NULL, NULL);

      cell_state = flags & BTK_CELL_RENDERER_SELECTED ? BTK_STATE_SELECTED :
	      (flags & BTK_CELL_RENDERER_PRELIT ? BTK_STATE_PRELIGHT :
	      (flags & BTK_CELL_RENDERER_INSENSITIVE ? BTK_STATE_INSENSITIVE : BTK_STATE_NORMAL));
      btk_paint_focus (tree_column->tree_view->style,
		       window,
		       cell_state,
		       cell_area,
		       tree_column->tree_view,
		       "treeview",
		       focus_rectangle.x,
		       focus_rectangle.y,
		       focus_rectangle.width,
		       focus_rectangle.height);
    }
}

/**
 * btk_tree_view_column_cell_is_visible:
 * @tree_column: A #BtkTreeViewColumn
 * 
 * Returns %TRUE if any of the cells packed into the @tree_column are visible.
 * For this to be meaningful, you must first initialize the cells with
 * btk_tree_view_column_cell_set_cell_data()
 * 
 * Return value: %TRUE, if any of the cells packed into the @tree_column are currently visible
 **/
gboolean
btk_tree_view_column_cell_is_visible (BtkTreeViewColumn *tree_column)
{
  GList *list;

  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), FALSE);

  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) list->data;

      if (info->cell->visible)
	return TRUE;
    }

  return FALSE;
}

/**
 * btk_tree_view_column_focus_cell:
 * @tree_column: A #BtkTreeViewColumn
 * @cell: A #BtkCellRenderer
 *
 * Sets the current keyboard focus to be at @cell, if the column contains
 * 2 or more editable and activatable cells.
 *
 * Since: 2.2
 **/
void
btk_tree_view_column_focus_cell (BtkTreeViewColumn *tree_column,
				 BtkCellRenderer   *cell)
{
  GList *list;
  gboolean found_cell = FALSE;

  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));
  g_return_if_fail (BTK_IS_CELL_RENDERER (cell));

  if (_btk_tree_view_column_count_special_cells (tree_column) < 2)
    return;

  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = list->data;

      if (info->cell == cell)
        {
	  info->has_focus = TRUE;
	  found_cell = TRUE;
	  break;
	}
    }

  if (found_cell)
    {
      for (list = tree_column->cell_list; list; list = list->next)
        {
	  BtkTreeViewColumnCellInfo *info = list->data;

	  if (info->cell != cell)
	    info->has_focus = FALSE;
        }

      /* FIXME: redraw? */
    }
}

void
_btk_tree_view_column_cell_set_dirty (BtkTreeViewColumn *tree_column,
				      gboolean           install_handler)
{
  GList *list;

  for (list = tree_column->cell_list; list; list = list->next)
    {
      BtkTreeViewColumnCellInfo *info = (BtkTreeViewColumnCellInfo *) list->data;

      info->requested_width = 0;
    }
  tree_column->dirty = TRUE;
  tree_column->requested_width = -1;
  tree_column->width = 0;

  if (tree_column->tree_view &&
      btk_widget_get_realized (tree_column->tree_view))
    {
      if (install_handler)
	_btk_tree_view_install_mark_rows_col_dirty (BTK_TREE_VIEW (tree_column->tree_view));
      else
	BTK_TREE_VIEW (tree_column->tree_view)->priv->mark_rows_col_dirty = TRUE;
      btk_widget_queue_resize (tree_column->tree_view);
    }
}

void
_btk_tree_view_column_start_editing (BtkTreeViewColumn *tree_column,
				     BtkCellEditable   *cell_editable)
{
  g_return_if_fail (tree_column->editable_widget == NULL);

  tree_column->editable_widget = cell_editable;
}

void
_btk_tree_view_column_stop_editing (BtkTreeViewColumn *tree_column)
{
  GList *list;

  g_return_if_fail (tree_column->editable_widget != NULL);

  tree_column->editable_widget = NULL;
  for (list = tree_column->cell_list; list; list = list->next)
    ((BtkTreeViewColumnCellInfo *)list->data)->in_editing_mode = FALSE;
}

void
_btk_tree_view_column_get_neighbor_sizes (BtkTreeViewColumn *column,
					  BtkCellRenderer   *cell,
					  gint              *left,
					  gint              *right)
{
  GList *list;
  BtkTreeViewColumnCellInfo *info;
  gint l, r;
  gboolean rtl;

  l = r = 0;

  list = btk_tree_view_column_cell_first (column);  

  while (list)
    {
      info = (BtkTreeViewColumnCellInfo *)list->data;
      
      list = btk_tree_view_column_cell_next (column, list);

      if (info->cell == cell)
	break;
      
      if (info->cell->visible)
	l += info->real_width + column->spacing;
    }

  while (list)
    {
      info = (BtkTreeViewColumnCellInfo *)list->data;
      
      list = btk_tree_view_column_cell_next (column, list);

      if (info->cell->visible)
	r += info->real_width + column->spacing;
    }

  rtl = (btk_widget_get_direction (BTK_WIDGET (column->tree_view)) == BTK_TEXT_DIR_RTL);
  if (left)
    *left = rtl ? r : l;

  if (right)
    *right = rtl ? l : r;
}

/**
 * btk_tree_view_column_cell_get_position:
 * @tree_column: a #BtkTreeViewColumn
 * @cell_renderer: a #BtkCellRenderer
 * @start_pos: return location for the horizontal position of @cell within
 *            @tree_column, may be %NULL
 * @width: return location for the width of @cell, may be %NULL
 *
 * Obtains the horizontal position and size of a cell in a column. If the
 * cell is not found in the column, @start_pos and @width are not changed and
 * %FALSE is returned.
 * 
 * Return value: %TRUE if @cell belongs to @tree_column.
 */
gboolean
btk_tree_view_column_cell_get_position (BtkTreeViewColumn *tree_column,
					BtkCellRenderer   *cell_renderer,
					gint              *start_pos,
					gint              *width)
{
  GList *list;
  gint current_x = 0;
  gboolean found_cell = FALSE;
  BtkTreeViewColumnCellInfo *cellinfo = NULL;

  list = btk_tree_view_column_cell_first (tree_column);
  for (; list; list = btk_tree_view_column_cell_next (tree_column, list))
    {
      cellinfo = list->data;
      if (cellinfo->cell == cell_renderer)
        {
          found_cell = TRUE;
          break;
        }

      if (cellinfo->cell->visible)
        current_x += cellinfo->real_width;
    }

  if (found_cell)
    {
      if (start_pos)
        *start_pos = current_x;
      if (width)
        *width = cellinfo->real_width;
    }

  return found_cell;
}

/**
 * btk_tree_view_column_queue_resize:
 * @tree_column: A #BtkTreeViewColumn
 *
 * Flags the column, and the cell renderers added to this column, to have
 * their sizes renegotiated.
 *
 * Since: 2.8
 **/
void
btk_tree_view_column_queue_resize (BtkTreeViewColumn *tree_column)
{
  g_return_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column));

  if (tree_column->tree_view)
    _btk_tree_view_column_cell_set_dirty (tree_column, TRUE);
}

/**
 * btk_tree_view_column_get_tree_view:
 * @tree_column: A #BtkTreeViewColumn
 *
 * Returns the #BtkTreeView wherein @tree_column has been inserted.
 * If @column is currently not inserted in any tree view, %NULL is
 * returned.
 *
 * Return value: (transfer none): The tree view wherein @column has
 *     been inserted if any, %NULL otherwise.
 *
 * Since: 2.12
 */
BtkWidget *
btk_tree_view_column_get_tree_view (BtkTreeViewColumn *tree_column)
{
  g_return_val_if_fail (BTK_IS_TREE_VIEW_COLUMN (tree_column), NULL);

  return tree_column->tree_view;
}

#define __BTK_TREE_VIEW_COLUMN_C__
#include "btkaliasdef.c"
