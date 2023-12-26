/* BtkPrinterOptionWidget
 * Copyright (C) 2006 Alexander Larsson  <alexl@redhat.com>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "btkintl.h"
#include "btkalignment.h"
#include "btkcheckbutton.h"
#include "btkcelllayout.h"
#include "btkcellrenderertext.h"
#include "btkcombobox.h"
#include "btkfilechooserbutton.h"
#include "btkimage.h"
#include "btklabel.h"
#include "btkliststore.h"
#include "btkradiobutton.h"
#include "btkstock.h"
#include "btktable.h"
#include "btktogglebutton.h"
#include "btkprivate.h"

#include "btkprinteroptionwidget.h"
#include "btkalias.h"

#define BTK_PRINTER_OPTION_WIDGET_GET_PRIVATE(o)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((o), BTK_TYPE_PRINTER_OPTION_WIDGET, BtkPrinterOptionWidgetPrivate))

static void btk_printer_option_widget_finalize (GObject *object);

static void deconstruct_widgets (BtkPrinterOptionWidget *widget);
static void construct_widgets (BtkPrinterOptionWidget *widget);
static void update_widgets (BtkPrinterOptionWidget *widget);

struct BtkPrinterOptionWidgetPrivate
{
  BtkPrinterOption *source;
  gulong source_changed_handler;
  
  BtkWidget *check;
  BtkWidget *combo;
  BtkWidget *entry;
  BtkWidget *image;
  BtkWidget *label;
  BtkWidget *filechooser;
  BtkWidget *box;
};

enum {
  CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_SOURCE
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkPrinterOptionWidget, btk_printer_option_widget, BTK_TYPE_HBOX)

static void btk_printer_option_widget_set_property (GObject      *object,
						    guint         prop_id,
						    const GValue *value,
						    GParamSpec   *pspec);
static void btk_printer_option_widget_get_property (GObject      *object,
						    guint         prop_id,
						    GValue       *value,
						    GParamSpec   *pspec);
static gboolean btk_printer_option_widget_mnemonic_activate (BtkWidget *widget,
							      gboolean  group_cycling);

static void
btk_printer_option_widget_class_init (BtkPrinterOptionWidgetClass *class)
{
  GObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (GObjectClass *) class;
  widget_class = (BtkWidgetClass *) class;

  object_class->finalize = btk_printer_option_widget_finalize;
  object_class->set_property = btk_printer_option_widget_set_property;
  object_class->get_property = btk_printer_option_widget_get_property;

  widget_class->mnemonic_activate = btk_printer_option_widget_mnemonic_activate;

  g_type_class_add_private (class, sizeof (BtkPrinterOptionWidgetPrivate));  

  signals[CHANGED] =
    g_signal_new ("changed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BtkPrinterOptionWidgetClass, changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class,
                                   PROP_SOURCE,
                                   g_param_spec_object ("source",
							P_("Source option"),
							P_("The PrinterOption backing this widget"),
							BTK_TYPE_PRINTER_OPTION,
							BTK_PARAM_READWRITE | G_PARAM_CONSTRUCT));

}

static void
btk_printer_option_widget_init (BtkPrinterOptionWidget *widget)
{
  widget->priv = BTK_PRINTER_OPTION_WIDGET_GET_PRIVATE (widget); 

  btk_box_set_spacing (BTK_BOX (widget), 12);
}

static void
btk_printer_option_widget_finalize (GObject *object)
{
  BtkPrinterOptionWidget *widget = BTK_PRINTER_OPTION_WIDGET (object);
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  
  if (priv->source)
    {
      g_object_unref (priv->source);
      priv->source = NULL;
    }
  
  G_OBJECT_CLASS (btk_printer_option_widget_parent_class)->finalize (object);
}

static void
btk_printer_option_widget_set_property (GObject         *object,
					guint            prop_id,
					const GValue    *value,
					GParamSpec      *pspec)
{
  BtkPrinterOptionWidget *widget;
  
  widget = BTK_PRINTER_OPTION_WIDGET (object);

  switch (prop_id)
    {
    case PROP_SOURCE:
      btk_printer_option_widget_set_source (widget, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_printer_option_widget_get_property (GObject    *object,
					guint       prop_id,
					GValue     *value,
					GParamSpec *pspec)
{
  BtkPrinterOptionWidget *widget = BTK_PRINTER_OPTION_WIDGET (object);
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;

  switch (prop_id)
    {
    case PROP_SOURCE:
      g_value_set_object (value, priv->source);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
btk_printer_option_widget_mnemonic_activate (BtkWidget *widget,
					     gboolean   group_cycling)
{
  BtkPrinterOptionWidget *powidget = BTK_PRINTER_OPTION_WIDGET (widget);
  BtkPrinterOptionWidgetPrivate *priv = powidget->priv;

  if (priv->check)
    return btk_widget_mnemonic_activate (priv->check, group_cycling);
  if (priv->combo)
    return btk_widget_mnemonic_activate (priv->combo, group_cycling);
  if (priv->entry)
    return btk_widget_mnemonic_activate (priv->entry, group_cycling);

  return FALSE;
}

static void
emit_changed (BtkPrinterOptionWidget *widget)
{
  g_signal_emit (widget, signals[CHANGED], 0);
}

BtkWidget *
btk_printer_option_widget_new (BtkPrinterOption *source)
{
  return g_object_new (BTK_TYPE_PRINTER_OPTION_WIDGET, "source", source, NULL);
}

static void
source_changed_cb (BtkPrinterOption *source,
		   BtkPrinterOptionWidget  *widget)
{
  update_widgets (widget);
  emit_changed (widget);
}

void
btk_printer_option_widget_set_source (BtkPrinterOptionWidget *widget,
				      BtkPrinterOption       *source)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;

  if (source)
    g_object_ref (source);
  
  if (priv->source)
    {
      g_signal_handler_disconnect (priv->source,
				   priv->source_changed_handler);
      g_object_unref (priv->source);
    }

  priv->source = source;

  if (source)
    priv->source_changed_handler =
      g_signal_connect (source, "changed", G_CALLBACK (source_changed_cb), widget);

  construct_widgets (widget);
  update_widgets (widget);

  g_object_notify (G_OBJECT (widget), "source");
}

enum {
  NAME_COLUMN,
  VALUE_COLUMN,
  N_COLUMNS
};

static void
combo_box_set_model (BtkWidget *combo_box)
{
  BtkListStore *store;

  store = btk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
  btk_combo_box_set_model (BTK_COMBO_BOX (combo_box), BTK_TREE_MODEL (store));
  g_object_unref (store);
}

static void
combo_box_set_view (BtkWidget *combo_box)
{
  BtkCellRenderer *cell;

  cell = btk_cell_renderer_text_new ();
  btk_cell_layout_pack_start (BTK_CELL_LAYOUT (combo_box), cell, TRUE);
  btk_cell_layout_set_attributes (BTK_CELL_LAYOUT (combo_box), cell,
                                  "text", NAME_COLUMN,
                                   NULL);
}

static BtkWidget *
combo_box_entry_new (void)
{
  BtkWidget *combo_box;
  combo_box = g_object_new (BTK_TYPE_COMBO_BOX, "has-entry", TRUE, NULL);

  combo_box_set_model (combo_box);

  btk_combo_box_set_entry_text_column (BTK_COMBO_BOX (combo_box), NAME_COLUMN);

  return combo_box;
}

static BtkWidget *
combo_box_new (void)
{
  BtkWidget *combo_box;
  combo_box = btk_combo_box_new ();

  combo_box_set_model (combo_box);
  combo_box_set_view (combo_box);

  return combo_box;
}
  
static void
combo_box_append (BtkWidget   *combo,
		  const gchar *display_text,
		  const gchar *value)
{
  BtkTreeModel *model;
  BtkListStore *store;
  BtkTreeIter iter;
  
  model = btk_combo_box_get_model (BTK_COMBO_BOX (combo));
  store = BTK_LIST_STORE (model);

  btk_list_store_append (store, &iter);
  btk_list_store_set (store, &iter,
		      NAME_COLUMN, display_text,
		      VALUE_COLUMN, value,
		      -1);
}

struct ComboSet {
  BtkComboBox *combo;
  const gchar *value;
};

static gboolean
set_cb (BtkTreeModel *model, 
	BtkTreePath  *path, 
	BtkTreeIter  *iter, 
	gpointer      data)
{
  struct ComboSet *set_data = data;
  gboolean found;
  char *value;
  
  btk_tree_model_get (model, iter, VALUE_COLUMN, &value, -1);
  found = (strcmp (value, set_data->value) == 0);
  g_free (value);
  
  if (found)
    btk_combo_box_set_active_iter (set_data->combo, iter);

  return found;
}

static void
combo_box_set (BtkWidget   *combo,
	       const gchar *value)
{
  BtkTreeModel *model;
  struct ComboSet set_data;
  
  model = btk_combo_box_get_model (BTK_COMBO_BOX (combo));

  set_data.combo = BTK_COMBO_BOX (combo);
  set_data.value = value;
  btk_tree_model_foreach (model, set_cb, &set_data);
}

static gchar *
combo_box_get (BtkWidget *combo, gboolean *custom)
{
  BtkTreeModel *model;
  gchar *value;
  BtkTreeIter iter;

  model = btk_combo_box_get_model (BTK_COMBO_BOX (combo));

  value = NULL;
  if (btk_combo_box_get_active_iter (BTK_COMBO_BOX (combo), &iter))
    {
      btk_tree_model_get (model, &iter, VALUE_COLUMN, &value, -1);
      *custom = FALSE;
    }
  else
    {
      if (btk_combo_box_get_has_entry (BTK_COMBO_BOX (combo)))
        {
          value = g_strdup (btk_entry_get_text (BTK_ENTRY (btk_bin_get_child (BTK_BIN (combo)))));
          *custom = TRUE;
        }

      if (!value || !btk_tree_model_get_iter_first (model, &iter))
        return value;

      /* If the user entered an item from the dropdown list manually, return
       * the non-custom option instead. */
      do
        {
          gchar *val, *name;
          btk_tree_model_get (model, &iter, VALUE_COLUMN, &val,
                                            NAME_COLUMN, &name, -1);
          if (g_str_equal (value, name))
            {
              *custom = FALSE;
              g_free (name);
              g_free (value);
              return val;
            }

          g_free (val);
          g_free (name);
        }
      while (btk_tree_model_iter_next (model, &iter));
    }

  return value;
}


static void
deconstruct_widgets (BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;

  if (priv->check)
    {
      btk_widget_destroy (priv->check);
      priv->check = NULL;
    }
  
  if (priv->combo)
    {
      btk_widget_destroy (priv->combo);
      priv->combo = NULL;
    }
  
  if (priv->entry)
    {
      btk_widget_destroy (priv->entry);
      priv->entry = NULL;
    }

  /* make sure entry and combo are destroyed first */
  /* as we use the two of them to create the filechooser */
  if (priv->filechooser)
    {
      btk_widget_destroy (priv->filechooser);
      priv->filechooser = NULL;
    }

  if (priv->image)
    {
      btk_widget_destroy (priv->image);
      priv->image = NULL;
    }

  if (priv->label)
    {
      btk_widget_destroy (priv->label);
      priv->label = NULL;
    }
}

static void
check_toggled_cb (BtkToggleButton        *toggle_button,
		  BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  btk_printer_option_set_boolean (priv->source,
				  btk_toggle_button_get_active (toggle_button));
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
filesave_changed_cb (BtkWidget              *button,
                     BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *uri, *file;
  gchar *directory;

  file = g_filename_from_utf8 (btk_entry_get_text (BTK_ENTRY (priv->entry)),
			       -1, NULL, NULL, NULL);
  if (file == NULL)
    return;

  /* combine the value of the chooser with the value of the entry */
  g_signal_handler_block (priv->source, priv->source_changed_handler);  

  directory = btk_file_chooser_get_filename (BTK_FILE_CHOOSER (priv->combo));

  if ((g_uri_parse_scheme (file) == NULL) && (directory != NULL))
    {
      if (g_path_is_absolute (file))
        uri = g_filename_to_uri (file, NULL, NULL);
      else
        {
          gchar *path;

#ifdef G_OS_UNIX
          if (file[0] == '~' && file[1] == '/')
            {
              path = g_build_filename (g_get_home_dir (), file + 2, NULL);
            }
          else
#endif
            {
              path = g_build_filename (directory, file, NULL);
            }

          uri = g_filename_to_uri (path, NULL, NULL);

          g_free (path);
        }
    }
  else
    {
      if (g_uri_parse_scheme (file) != NULL)
        uri = g_strdup (file);
      else
        {
          gchar *chooser_uri = btk_file_chooser_get_uri (BTK_FILE_CHOOSER (priv->combo));
          if (chooser_uri)
            {
              uri = g_build_path ("/", chooser_uri, file, NULL);
              g_free (chooser_uri);
            }
          else
            uri = g_filename_to_uri (file, NULL, NULL);
        }
    }
 
  if (uri)
    btk_printer_option_set (priv->source, uri);

  g_free (uri);
  g_free (file);
  g_free (directory);

  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static gchar *
filter_numeric (const gchar *val,
                gboolean     allow_neg,
		gboolean     allow_dec,
                gboolean    *changed_out)
{
  gchar *filtered_val;
  int i, j;
  int len = strlen (val);
  gboolean dec_set = FALSE;

  filtered_val = g_malloc (len + 1);

  for (i = 0, j = 0; i < len; i++)
    {
      if (isdigit (val[i]))
        {
          filtered_val[j] = val[i];
	  j++;
	}
      else if (allow_dec && !dec_set && 
               (val[i] == '.' || val[i] == ','))
        {
	  /* allow one period or comma
	   * we should be checking locals
	   * but this is good enough for now
	   */
          filtered_val[j] = val[i];
	  dec_set = TRUE;
	  j++;
	}
      else if (allow_neg && i == 0 && val[0] == '-')
        {
          filtered_val[0] = val[0];
	  j++;
	}
    }

  filtered_val[j] = '\0';
  *changed_out = !(i == j);

  return filtered_val;
}

static void
combo_changed_cb (BtkWidget              *combo,
		  BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *value;
  gchar *filtered_val = NULL;
  gboolean changed;
  gboolean custom = TRUE;

  g_signal_handler_block (priv->source, priv->source_changed_handler);
  
  value = combo_box_get (combo, &custom);

  /* Handle constraints if the user entered a custom value. */
  if (custom)
    {
      switch (priv->source->type)
        {
        case BTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
          filtered_val = filter_numeric (value, FALSE, FALSE, &changed);
          break;
        case BTK_PRINTER_OPTION_TYPE_PICKONE_INT:
          filtered_val = filter_numeric (value, TRUE, FALSE, &changed);
          break;
        case BTK_PRINTER_OPTION_TYPE_PICKONE_REAL:
          filtered_val = filter_numeric (value, TRUE, TRUE, &changed);
          break;
        default:
          break;
        }
    }

  if (filtered_val)
    {
      g_free (value);

      if (changed)
        {
          BtkEntry *entry;
	  
	  entry = BTK_ENTRY (btk_bin_get_child (BTK_BIN (combo)));

          btk_entry_set_text (entry, filtered_val);
	}
      value = filtered_val;
    }

  if (value)
    btk_printer_option_set (priv->source, value);
  g_free (value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
entry_changed_cb (BtkWidget              *entry,
		  BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  const gchar *value;
  
  g_signal_handler_block (priv->source, priv->source_changed_handler);
  value = btk_entry_get_text (BTK_ENTRY (entry));
  if (value)
    btk_printer_option_set (priv->source, value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}


static void
radio_changed_cb (BtkWidget              *button,
		  BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  gchar *value;
  
  g_signal_handler_block (priv->source, priv->source_changed_handler);
  value = g_object_get_data (G_OBJECT (button), "value");
  if (value)
    btk_printer_option_set (priv->source, value);
  g_signal_handler_unblock (priv->source, priv->source_changed_handler);
  emit_changed (widget);
}

static void
select_maybe (BtkWidget   *widget, 
	      const gchar *value)
{
  gchar *v = g_object_get_data (G_OBJECT (widget), "value");
      
  if (strcmp (value, v) == 0)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (widget), TRUE);
}

static void
alternative_set (BtkWidget   *box,
		 const gchar *value)
{
  btk_container_foreach (BTK_CONTAINER (box), 
			 (BtkCallback) select_maybe,
			 (gpointer) value);
}

static GSList *
alternative_append (BtkWidget              *box,
		    const gchar            *label,
                    const gchar            *value,
		    BtkPrinterOptionWidget *widget,
		    GSList                 *group)
{
  BtkWidget *button;

  button = btk_radio_button_new_with_label (group, label);
  btk_widget_show (button);
  btk_box_pack_start (BTK_BOX (box), button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (button), "value", (gpointer)value);
  g_signal_connect (button, "toggled", 
		    G_CALLBACK (radio_changed_cb), widget);

  return btk_radio_button_get_group (BTK_RADIO_BUTTON (button));
}

static void
construct_widgets (BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  BtkPrinterOption *source;
  char *text;
  int i;
  GSList *group;

  source = priv->source;
  
  deconstruct_widgets (widget);
  
  btk_widget_set_sensitive (BTK_WIDGET (widget), TRUE);

  if (source == NULL)
    {
      priv->combo = combo_box_new ();
      combo_box_append (priv->combo,_("Not available"), "None");
      btk_combo_box_set_active (BTK_COMBO_BOX (priv->combo), 0);
      btk_widget_set_sensitive (BTK_WIDGET (widget), FALSE);
      btk_widget_show (priv->combo);
      btk_box_pack_start (BTK_BOX (widget), priv->combo, TRUE, TRUE, 0);
    }
  else switch (source->type)
    {
    case BTK_PRINTER_OPTION_TYPE_BOOLEAN:
      priv->check = btk_check_button_new_with_mnemonic (source->display_text);
      g_signal_connect (priv->check, "toggled", G_CALLBACK (check_toggled_cb), widget);
      btk_widget_show (priv->check);
      btk_box_pack_start (BTK_BOX (widget), priv->check, TRUE, TRUE, 0);
      break;
    case BTK_PRINTER_OPTION_TYPE_PICKONE:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_REAL:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_INT:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_STRING:
      if (source->type == BTK_PRINTER_OPTION_TYPE_PICKONE)
        {
          priv->combo = combo_box_new ();
	}
      else
        {
          priv->combo = combo_box_entry_new ();

          if (source->type == BTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD ||
	      source->type == BTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE)
	    {
              BtkEntry *entry;

	      entry = BTK_ENTRY (btk_bin_get_child (BTK_BIN (priv->combo)));

              btk_entry_set_visibility (entry, FALSE); 
	    }
        }
       

      for (i = 0; i < source->num_choices; i++)
	  combo_box_append (priv->combo,
			    source->choices_display[i],
			    source->choices[i]);
      btk_widget_show (priv->combo);
      btk_box_pack_start (BTK_BOX (widget), priv->combo, TRUE, TRUE, 0);
      g_signal_connect (priv->combo, "changed", G_CALLBACK (combo_changed_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = btk_label_new_with_mnemonic (text);
      g_free (text);
      btk_widget_show (priv->label);
      break;

    case BTK_PRINTER_OPTION_TYPE_ALTERNATIVE:
      group = NULL;
      priv->box = btk_hbox_new (FALSE, 12);
      btk_widget_show (priv->box);
      btk_box_pack_start (BTK_BOX (widget), priv->box, TRUE, TRUE, 0);
      for (i = 0; i < source->num_choices; i++)
	group = alternative_append (priv->box,
				    source->choices_display[i],
				    source->choices[i],
				    widget,
				    group);

      if (source->display_text)
	{
	  text = g_strdup_printf ("%s:", source->display_text);
	  priv->label = btk_label_new_with_mnemonic (text);
	  g_free (text);
	  btk_widget_show (priv->label);
	}
      break;

    case BTK_PRINTER_OPTION_TYPE_STRING:
      priv->entry = btk_entry_new ();
      btk_entry_set_activates_default (BTK_ENTRY (priv->entry),
                                       btk_printer_option_get_activates_default (source));
      btk_widget_show (priv->entry);
      btk_box_pack_start (BTK_BOX (widget), priv->entry, TRUE, TRUE, 0);
      g_signal_connect (priv->entry, "changed", G_CALLBACK (entry_changed_cb), widget);

      text = g_strdup_printf ("%s:", source->display_text);
      priv->label = btk_label_new_with_mnemonic (text);
      g_free (text);
      btk_widget_show (priv->label);

      break;

    case BTK_PRINTER_OPTION_TYPE_FILESAVE:
      {
        BtkWidget *label;
        
        priv->filechooser = btk_table_new (2, 2, FALSE);
        btk_table_set_row_spacings (BTK_TABLE (priv->filechooser), 6);
        btk_table_set_col_spacings (BTK_TABLE (priv->filechooser), 12);

        /* TODO: make this a btkfilechooserentry once we move to BTK */
        priv->entry = btk_entry_new ();
        priv->combo = btk_file_chooser_button_new (source->display_text,
                                                   BTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

        g_object_set (priv->combo, "local-only", FALSE, NULL);
        btk_entry_set_activates_default (BTK_ENTRY (priv->entry),
                                         btk_printer_option_get_activates_default (source));

        label = btk_label_new_with_mnemonic (_("_Name:"));
        btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
        btk_label_set_mnemonic_widget (BTK_LABEL (label), priv->entry);

        btk_table_attach (BTK_TABLE (priv->filechooser), label,
                          0, 1, 0, 1, BTK_FILL, 0,
                          0, 0);

        btk_table_attach (BTK_TABLE (priv->filechooser), priv->entry,
                          1, 2, 0, 1, BTK_FILL, 0,
                          0, 0);

        label = btk_label_new_with_mnemonic (_("_Save in folder:"));
        btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
        btk_label_set_mnemonic_widget (BTK_LABEL (label), priv->combo);

        btk_table_attach (BTK_TABLE (priv->filechooser), label,
                          0, 1, 1, 2, BTK_FILL, 0,
                          0, 0);

        btk_table_attach (BTK_TABLE (priv->filechooser), priv->combo,
                          1, 2, 1, 2, BTK_FILL, 0,
                          0, 0);

        btk_widget_show_all (priv->filechooser);
        btk_box_pack_start (BTK_BOX (widget), priv->filechooser, TRUE, TRUE, 0);

        g_signal_connect (priv->entry, "changed", G_CALLBACK (filesave_changed_cb), widget);

        g_signal_connect (priv->combo, "selection-changed", G_CALLBACK (filesave_changed_cb), widget);
      }
      break;
    default:
      break;
    }

  priv->image = btk_image_new_from_stock (BTK_STOCK_DIALOG_WARNING, BTK_ICON_SIZE_MENU);
  btk_box_pack_start (BTK_BOX (widget), priv->image, FALSE, FALSE, 0);
}

static void
update_widgets (BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;
  BtkPrinterOption *source;

  source = priv->source;
  
  if (source == NULL)
    {
      btk_widget_hide (priv->image);
      return;
    }

  switch (source->type)
    {
    case BTK_PRINTER_OPTION_TYPE_BOOLEAN:
      if (g_ascii_strcasecmp (source->value, "True") == 0)
	btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->check), TRUE);
      else
	btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (priv->check), FALSE);
      break;
    case BTK_PRINTER_OPTION_TYPE_PICKONE:
      combo_box_set (priv->combo, source->value);
      break;
    case BTK_PRINTER_OPTION_TYPE_ALTERNATIVE:
      alternative_set (priv->box, source->value);
      break;
    case BTK_PRINTER_OPTION_TYPE_STRING:
      btk_entry_set_text (BTK_ENTRY (priv->entry), source->value);
      break;
    case BTK_PRINTER_OPTION_TYPE_PICKONE_PASSWORD:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_PASSCODE:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_REAL:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_INT:
    case BTK_PRINTER_OPTION_TYPE_PICKONE_STRING:
      {
        BtkEntry *entry;

        entry = BTK_ENTRY (btk_bin_get_child (BTK_BIN (priv->combo)));
        if (btk_printer_option_has_choice (source, source->value))
          combo_box_set (priv->combo, source->value);
        else
          btk_entry_set_text (entry, source->value);

        break;
      }
    case BTK_PRINTER_OPTION_TYPE_FILESAVE:
      {
        gchar *filename = g_filename_from_uri (source->value, NULL, NULL);
        if (filename != NULL)
          {
            gchar *basename, *dirname, *text;

            basename = g_path_get_basename (filename);
            dirname = g_path_get_dirname (filename);
            text = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);

            if (text != NULL)
              btk_entry_set_text (BTK_ENTRY (priv->entry), text);
            if (g_path_is_absolute (dirname))
              btk_file_chooser_set_current_folder (BTK_FILE_CHOOSER (priv->combo),
                                                   dirname);
            g_free (text);
            g_free (basename);
            g_free (dirname);
            g_free (filename);
          }
	else
	  btk_entry_set_text (BTK_ENTRY (priv->entry), source->value);
	break;
      }
    default:
      break;
    }

  if (source->has_conflict)
    btk_widget_show (priv->image);
  else
    btk_widget_hide (priv->image);
}

gboolean
btk_printer_option_widget_has_external_label (BtkPrinterOptionWidget *widget)
{
  return widget->priv->label != NULL;
}

BtkWidget *
btk_printer_option_widget_get_external_label (BtkPrinterOptionWidget  *widget)
{
  return widget->priv->label;
}

const gchar *
btk_printer_option_widget_get_value (BtkPrinterOptionWidget *widget)
{
  BtkPrinterOptionWidgetPrivate *priv = widget->priv;

  if (priv->source)
    return priv->source->value;
  
  return "";
}

#define __BTK_PRINTER_OPTION_WIDGET_C__
#include "btkaliasdef.c"
