/* prop-editor.c
 * Copyright (C) 2000  Red Hat, Inc.
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

#include <string.h>

#include <btk/btk.h>

#include "prop-editor.h"


typedef struct
{
  bpointer instance;
  BObject *alive_object;
  buint id;
} DisconnectData;

static void
disconnect_func (bpointer data)
{
  DisconnectData *dd = data;
  
  g_signal_handler_disconnect (dd->instance, dd->id);
}

static void
signal_removed (bpointer  data,
		GClosure *closure)
{
  DisconnectData *dd = data;

  g_object_steal_data (dd->alive_object, "alive-object-data");
  g_free (dd);
}

static bboolean
is_child_property (BParamSpec *pspec)
{
  return g_param_spec_get_qdata (pspec, g_quark_from_string ("is-child-prop")) != NULL;
}

static void
mark_child_property (BParamSpec *pspec)
{
  g_param_spec_set_qdata (pspec, g_quark_from_string ("is-child-prop"), 
			  BINT_TO_POINTER (TRUE));
}

static void
g_object_connect_property (BObject     *object,
			   BParamSpec  *spec,
                           GCallback    func,
                           bpointer     data,
                           BObject     *alive_object)
{
  GClosure *closure;
  bchar *with_detail;
  DisconnectData *dd;

  if (is_child_property (spec))
    with_detail = g_strconcat ("child-notify::", spec->name, NULL);
  else
    with_detail = g_strconcat ("notify::", spec->name, NULL);

  dd = g_new (DisconnectData, 1);

  closure = g_cclosure_new (func, data, NULL);

  g_closure_add_invalidate_notifier (closure, dd, signal_removed);

  dd->id = g_signal_connect_closure (object, with_detail,
				     closure, FALSE);

  dd->instance = object;
  dd->alive_object = alive_object;
  
  g_object_set_data_full (B_OBJECT (alive_object),
                          "alive-object-data",
                          dd,
                          disconnect_func);
  
  g_free (with_detail);
}

typedef struct 
{
  BObject *obj;
  BParamSpec *spec;
  bint modified_id;
} ObjectProperty;

static void
free_object_property (ObjectProperty *p)
{
  g_free (p);
}

static void
connect_controller (BObject     *controller,
                    const bchar *signal,
                    BObject     *model,
		    BParamSpec  *spec,
                    GCallback    func)
{
  ObjectProperty *p;

  p = g_new (ObjectProperty, 1);
  p->obj = model;
  p->spec = spec;

  p->modified_id = g_signal_connect_data (controller, signal, func, p,
					  (GClosureNotify)free_object_property,
					  0);
  g_object_set_data (controller, "object-property", p);
}

static void
block_controller (BObject *controller)
{
  ObjectProperty *p = g_object_get_data (controller, "object-property");

  if (p)
    g_signal_handler_block (controller, p->modified_id);
}

static void
unblock_controller (BObject *controller)
{
  ObjectProperty *p = g_object_get_data (controller, "object-property");

  if (p)
    g_signal_handler_unblock (controller, p->modified_id);
}

static void
int_modified (BtkAdjustment *adj, bpointer data)
{
  ObjectProperty *p = data;

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, (int) adj->value, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, (int) adj->value, NULL);
}

static void
get_property_value (BObject *object, BParamSpec *pspec, BValue *value)
{
  if (is_child_property (pspec))
    {
      BtkWidget *widget = BTK_WIDGET (object);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_get_property (BTK_CONTAINER (parent),
					widget, pspec->name, value);
    }
  else
    g_object_get_property (object, pspec->name, value);
}

static void
int_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkAdjustment *adj = BTK_ADJUSTMENT (data);
  BValue val = { 0, };  

  b_value_init (&val, B_TYPE_INT);

  get_property_value (object, pspec, &val);

  if (b_value_get_int (&val) != (int)adj->value)
    {
      block_controller (B_OBJECT (adj));
      btk_adjustment_set_value (adj, b_value_get_int (&val));
      unblock_controller (B_OBJECT (adj));
    }

  b_value_unset (&val);
}

static void
uint_modified (BtkAdjustment *adj, bpointer data)
{
  ObjectProperty *p = data;

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, (buint) adj->value, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, (buint) adj->value, NULL);
}

static void
uint_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkAdjustment *adj = BTK_ADJUSTMENT (data);
  BValue val = { 0, };  

  b_value_init (&val, B_TYPE_UINT);
  get_property_value (object, pspec, &val);

  if (b_value_get_uint (&val) != (buint)adj->value)
    {
      block_controller (B_OBJECT (adj));
      btk_adjustment_set_value (adj, b_value_get_uint (&val));
      unblock_controller (B_OBJECT (adj));
    }

  b_value_unset (&val);
}

static void
float_modified (BtkAdjustment *adj, bpointer data)
{
  ObjectProperty *p = data;

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, (float) adj->value, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, (float) adj->value, NULL);
}

static void
float_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkAdjustment *adj = BTK_ADJUSTMENT (data);
  BValue val = { 0, };  

  b_value_init (&val, B_TYPE_FLOAT);
  get_property_value (object, pspec, &val);

  if (b_value_get_float (&val) != (float) adj->value)
    {
      block_controller (B_OBJECT (adj));
      btk_adjustment_set_value (adj, b_value_get_float (&val));
      unblock_controller (B_OBJECT (adj));
    }

  b_value_unset (&val);
}

static void
double_modified (BtkAdjustment *adj, bpointer data)
{
  ObjectProperty *p = data;

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, (double) adj->value, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, (double) adj->value, NULL);
}

static void
double_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkAdjustment *adj = BTK_ADJUSTMENT (data);
  BValue val = { 0, };  

  b_value_init (&val, B_TYPE_DOUBLE);
  get_property_value (object, pspec, &val);

  if (b_value_get_double (&val) != adj->value)
    {
      block_controller (B_OBJECT (adj));
      btk_adjustment_set_value (adj, b_value_get_double (&val));
      unblock_controller (B_OBJECT (adj));
    }

  b_value_unset (&val);
}

static void
string_modified (BtkEntry *entry, bpointer data)
{
  ObjectProperty *p = data;
  const bchar *text;

  text = btk_entry_get_text (entry);

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, text, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, text, NULL);
}

static void
string_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkEntry *entry = BTK_ENTRY (data);
  BValue val = { 0, };  
  const bchar *str;
  const bchar *text;
  
  b_value_init (&val, B_TYPE_STRING);
  get_property_value (object, pspec, &val);

  str = b_value_get_string (&val);
  if (str == NULL)
    str = "";
  text = btk_entry_get_text (entry);

  if (strcmp (str, text) != 0)
    {
      block_controller (B_OBJECT (entry));
      btk_entry_set_text (entry, str);
      unblock_controller (B_OBJECT (entry));
    }
  
  b_value_unset (&val);
}

static void
bool_modified (BtkToggleButton *tb, bpointer data)
{
  ObjectProperty *p = data;

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, (int) tb->active, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, (int) tb->active, NULL);
}

static void
bool_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkToggleButton *tb = BTK_TOGGLE_BUTTON (data);
  BValue val = { 0, };  
  
  b_value_init (&val, B_TYPE_BOOLEAN);
  get_property_value (object, pspec, &val);

  if (b_value_get_boolean (&val) != tb->active)
    {
      block_controller (B_OBJECT (tb));
      btk_toggle_button_set_active (tb, b_value_get_boolean (&val));
      unblock_controller (B_OBJECT (tb));
    }

  btk_label_set_text (BTK_LABEL (BTK_BIN (tb)->child), b_value_get_boolean (&val) ?
                      "TRUE" : "FALSE");
  
  b_value_unset (&val);
}


static void
enum_modified (BtkComboBox *cb, bpointer data)
{
  ObjectProperty *p = data;
  bint i;
  GEnumClass *eclass;
  
  eclass = G_ENUM_CLASS (g_type_class_peek (p->spec->value_type));

  i = btk_combo_box_get_active (cb);

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, eclass->values[i].value, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, eclass->values[i].value, NULL);
}

static void
enum_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkComboBox *cb = BTK_COMBO_BOX (data);
  BValue val = { 0, };  
  GEnumClass *eclass;
  bint i;

  eclass = G_ENUM_CLASS (g_type_class_peek (pspec->value_type));
  
  b_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);

  i = 0;
  while (i < eclass->n_values)
    {
      if (eclass->values[i].value == b_value_get_enum (&val))
        break;
      ++i;
    }
  
  if (btk_combo_box_get_active (cb) != i)
    {
      block_controller (B_OBJECT (cb));
      btk_combo_box_set_active (cb, i);
      unblock_controller (B_OBJECT (cb));
    }
  
  b_value_unset (&val);

}

static void
flags_modified (BtkCheckButton *button, bpointer data)
{
  ObjectProperty *p = data;
  bboolean active;
  GFlagsClass *fclass;
  buint flags;
  bint i;
  
  fclass = G_FLAGS_CLASS (g_type_class_peek (p->spec->value_type));
  
  active = btk_toggle_button_get_active (BTK_TOGGLE_BUTTON (button));
  i = BPOINTER_TO_INT (g_object_get_data (B_OBJECT (button), "index"));

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_get (BTK_CONTAINER (parent), 
			       widget, p->spec->name, &flags, NULL);
      if (active)
        flags |= fclass->values[i].value;
      else
        flags &= ~fclass->values[i].value;

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, flags, NULL);
    }
  else
    {
      g_object_get (p->obj, p->spec->name, &flags, NULL);

      if (active)
        flags |= fclass->values[i].value;
      else
        flags &= ~fclass->values[i].value;

      g_object_set (p->obj, p->spec->name, flags, NULL);
    }
}

static void
flags_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  GList *children, *c;
  BValue val = { 0, };  
  GFlagsClass *fclass;
  buint flags;
  bint i;

  fclass = G_FLAGS_CLASS (g_type_class_peek (pspec->value_type));
  
  b_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);
  flags = b_value_get_flags (&val);
  b_value_unset (&val);

  children = btk_container_get_children (BTK_CONTAINER (data));

  for (c = children, i = 0; c; c = c->next, i++)
    {
      block_controller (B_OBJECT (c->data));
      btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (c->data),
                                    (fclass->values[i].value & flags) != 0);
      unblock_controller (B_OBJECT (c->data));
    }

  g_list_free (children);
}

static gunichar
unichar_get_value (BtkEntry *entry)
{
  const bchar *text = btk_entry_get_text (entry);
  
  if (text[0])
    return g_utf8_get_char (text);
  else
    return 0;
}

static void
unichar_modified (BtkEntry *entry, bpointer data)
{
  ObjectProperty *p = data;
  gunichar val = unichar_get_value (entry);

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent), 
			       widget, p->spec->name, val, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, val, NULL);
}

static void
unichar_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkEntry *entry = BTK_ENTRY (data);
  gunichar new_val;
  gunichar old_val = unichar_get_value (entry);
  BValue val = { 0, };
  bchar buf[7];
  bint len;
  
  b_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);
  new_val = (gunichar)b_value_get_uint (&val);

  if (new_val != old_val)
    {
      if (!new_val)
	len = 0;
      else
	len = g_unichar_to_utf8 (new_val, buf);
      
      buf[len] = '\0';
      
      block_controller (B_OBJECT (entry));
      btk_entry_set_text (entry, buf);
      unblock_controller (B_OBJECT (entry));
    }
}

static void
pointer_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkLabel *label = BTK_LABEL (data);
  bchar *str;
  bpointer ptr;
  
  g_object_get (object, pspec->name, &ptr, NULL);

  str = g_strdup_printf ("Pointer: %p", ptr);
  btk_label_set_text (label, str);
  g_free (str);
}

static bchar *
object_label (BObject *obj, BParamSpec *pspec)
{
  const bchar *name;

  if (obj)
    name = g_type_name (B_TYPE_FROM_INSTANCE (obj));
  else if (pspec)
    name = g_type_name (G_PARAM_SPEC_VALUE_TYPE (pspec));
  else
    name = "unknown";
  return g_strdup_printf ("Object: %p (%s)", obj, name);
}

static void
object_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkWidget *label, *button;
  bchar *str;
  BObject *obj;
  
  GList *children = btk_container_get_children (BTK_CONTAINER (data)); 
  label = BTK_WIDGET (children->data);
  button = BTK_WIDGET (children->next->data);
  g_object_get (object, pspec->name, &obj, NULL);
  g_list_free (children);

  str = object_label (obj, pspec);
  
  btk_label_set_text (BTK_LABEL (label), str);
  btk_widget_set_sensitive (button, G_IS_OBJECT (obj));

  if (obj)
    g_object_unref (obj);

  g_free (str);
}

static void
model_destroy (bpointer data)
{
  g_object_steal_data (data, "model-object");
  btk_widget_destroy (data);
}

static void
window_destroy (bpointer data)
{
  g_object_steal_data (data, "prop-editor-win");
}

static void
object_properties (BtkWidget *button, 
		   BObject   *object)
{
  bchar *name;
  BObject *obj;

  name = (bchar *) g_object_get_data (B_OBJECT (button), "property-name");
  g_object_get (object, name, &obj, NULL);
  if (G_IS_OBJECT (obj)) 
    create_prop_editor (obj, 0);
}
 
static void
color_modified (BtkColorButton *cb, bpointer data)
{
  ObjectProperty *p = data;
  BdkColor color;

  btk_color_button_get_color (cb, &color);

  if (is_child_property (p->spec))
    {
      BtkWidget *widget = BTK_WIDGET (p->obj);
      BtkWidget *parent = btk_widget_get_parent (widget);

      btk_container_child_set (BTK_CONTAINER (parent),
			       widget, p->spec->name, &color, NULL);
    }
  else
    g_object_set (p->obj, p->spec->name, &color, NULL);
}

static void
color_changed (BObject *object, BParamSpec *pspec, bpointer data)
{
  BtkColorButton *cb = BTK_COLOR_BUTTON (data);
  BValue val = { 0, };
  BdkColor *color;
  BdkColor cb_color;

  b_value_init (&val, BDK_TYPE_COLOR);
  get_property_value (object, pspec, &val);

  color = b_value_get_boxed (&val);
  btk_color_button_get_color (cb, &cb_color);

  if (color != NULL && !bdk_color_equal (color, &cb_color))
    {
      block_controller (B_OBJECT (cb));
      btk_color_button_set_color (cb, color);
      unblock_controller (B_OBJECT (cb));
    }

  b_value_unset (&val);
}

static BtkWidget *
property_widget (BObject    *object, 
		 BParamSpec *spec, 
		 bboolean    can_modify)
{
  BtkWidget *prop_edit;
  BtkAdjustment *adj;
  bchar *msg;
  GType type = G_PARAM_SPEC_TYPE (spec);

  if (type == B_TYPE_PARAM_INT)
    {
      adj = BTK_ADJUSTMENT (btk_adjustment_new (G_PARAM_SPEC_INT (spec)->default_value,
						G_PARAM_SPEC_INT (spec)->minimum,
						G_PARAM_SPEC_INT (spec)->maximum,
						1,
						MAX ((G_PARAM_SPEC_INT (spec)->maximum -
						      G_PARAM_SPEC_INT (spec)->minimum) / 10, 1),
						0.0));
      
      prop_edit = btk_spin_button_new (adj, 1.0, 0);
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (int_changed), 
				 adj, B_OBJECT (adj));
      
      if (can_modify)
	connect_controller (B_OBJECT (adj), "value_changed",
			    object, spec, G_CALLBACK (int_modified));
    }
  else if (type == B_TYPE_PARAM_UINT)
    {
      adj = BTK_ADJUSTMENT (
			    btk_adjustment_new (G_PARAM_SPEC_UINT (spec)->default_value,
						G_PARAM_SPEC_UINT (spec)->minimum,
						G_PARAM_SPEC_UINT (spec)->maximum,
						1,
						MAX ((G_PARAM_SPEC_UINT (spec)->maximum -
						      G_PARAM_SPEC_UINT (spec)->minimum) / 10, 1),
						0.0));
      
      prop_edit = btk_spin_button_new (adj, 1.0, 0);
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (uint_changed), 
				 adj, B_OBJECT (adj));
      
      if (can_modify)
	connect_controller (B_OBJECT (adj), "value_changed",
			    object, spec, G_CALLBACK (uint_modified));
    }
  else if (type == B_TYPE_PARAM_FLOAT)
    {

      adj = BTK_ADJUSTMENT (btk_adjustment_new (G_PARAM_SPEC_FLOAT (spec)->default_value,
						G_PARAM_SPEC_FLOAT (spec)->minimum,
						G_PARAM_SPEC_FLOAT (spec)->maximum,
						0.1,
						MAX ((G_PARAM_SPEC_FLOAT (spec)->maximum -
						      G_PARAM_SPEC_FLOAT (spec)->minimum) / 10, 0.1),
						0.0));
      
      prop_edit = btk_spin_button_new (adj, 0.1, 2);
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (float_changed), 
				 adj, B_OBJECT (adj));
      
      if (can_modify)
	connect_controller (B_OBJECT (adj), "value_changed",
			    object, spec, G_CALLBACK (float_modified));
    }
  else if (type == B_TYPE_PARAM_DOUBLE)
    {
      adj = BTK_ADJUSTMENT (btk_adjustment_new (G_PARAM_SPEC_DOUBLE (spec)->default_value,
						G_PARAM_SPEC_DOUBLE (spec)->minimum,
						G_PARAM_SPEC_DOUBLE (spec)->maximum,
						0.1,
						MAX ((G_PARAM_SPEC_DOUBLE (spec)->maximum -
						      G_PARAM_SPEC_DOUBLE (spec)->minimum) / 10, 0.1),
						0.0));
      
      prop_edit = btk_spin_button_new (adj, 0.1, 2);
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (double_changed), 
				 adj, B_OBJECT (adj));
      
      if (can_modify)
	connect_controller (B_OBJECT (adj), "value_changed",
			    object, spec, G_CALLBACK (double_modified));
    }
  else if (type == B_TYPE_PARAM_STRING)
    {
      prop_edit = btk_entry_new ();
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (string_changed),
				 prop_edit, B_OBJECT (prop_edit));
      
      if (can_modify)
	connect_controller (B_OBJECT (prop_edit), "changed",
			    object, spec, G_CALLBACK (string_modified));
    }
  else if (type == B_TYPE_PARAM_BOOLEAN)
    {
      prop_edit = btk_toggle_button_new_with_label ("");
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (bool_changed),
				 prop_edit, B_OBJECT (prop_edit));
      
      if (can_modify)
	connect_controller (B_OBJECT (prop_edit), "toggled",
			    object, spec, G_CALLBACK (bool_modified));
    }
  else if (type == B_TYPE_PARAM_ENUM)
    {
      {
	GEnumClass *eclass;
	bint j;
	
	prop_edit = btk_combo_box_text_new ();
	
	eclass = G_ENUM_CLASS (g_type_class_ref (spec->value_type));
	
	j = 0;
	while (j < eclass->n_values)
	  {
	    btk_combo_box_text_append_text (BTK_COMBO_BOX_TEXT (prop_edit),
	                                    eclass->values[j].value_name);
	    ++j;
	  }
	
	g_type_class_unref (eclass);
	
	g_object_connect_property (object, spec,
				   G_CALLBACK (enum_changed),
				   prop_edit, B_OBJECT (prop_edit));
	
	if (can_modify)
	  connect_controller (B_OBJECT (prop_edit), "changed",
			      object, spec, G_CALLBACK (enum_modified));
      }
    }
  else if (type == B_TYPE_PARAM_FLAGS)
    {
      {
	GFlagsClass *fclass;
	bint j;
	
	prop_edit = btk_vbox_new (FALSE, 0);
	
	fclass = G_FLAGS_CLASS (g_type_class_ref (spec->value_type));
	
	for (j = 0; j < fclass->n_values; j++)
	  {
	    BtkWidget *b;
	    
	    b = btk_check_button_new_with_label (fclass->values[j].value_name);
            g_object_set_data (B_OBJECT (b), "index", BINT_TO_POINTER (j));
	    btk_widget_show (b);
	    btk_box_pack_start (BTK_BOX (prop_edit), b, FALSE, FALSE, 0);
	    if (can_modify) 
	      connect_controller (B_OBJECT (b), "toggled",
	     	                  object, spec, G_CALLBACK (flags_modified));
	  }
	
	g_type_class_unref (fclass);
	
	g_object_connect_property (object, spec,
				   G_CALLBACK (flags_changed),
				   prop_edit, B_OBJECT (prop_edit));
      }
    }
  else if (type == B_TYPE_PARAM_UNICHAR)
    {
      prop_edit = btk_entry_new ();
      btk_entry_set_max_length (BTK_ENTRY (prop_edit), 1);
      
      g_object_connect_property (object, spec,
				 G_CALLBACK (unichar_changed),
				 prop_edit, B_OBJECT (prop_edit));
      
      if (can_modify)
	connect_controller (B_OBJECT (prop_edit), "changed",
			    object, spec, G_CALLBACK (unichar_modified));
    }
  else if (type == B_TYPE_PARAM_POINTER)
    {
      prop_edit = btk_label_new ("");
      
      g_object_connect_property (object, spec, 
				 G_CALLBACK (pointer_changed),
				 prop_edit, B_OBJECT (prop_edit));
    }
  else if (type == B_TYPE_PARAM_OBJECT)
    {
      BtkWidget *label, *button;

      prop_edit = btk_hbox_new (FALSE, 5);

      label = btk_label_new ("");
      button = btk_button_new_with_label ("Properties");
      g_object_set_data (B_OBJECT (button), "property-name", spec->name);
      g_signal_connect (button, "clicked", 
			G_CALLBACK (object_properties), 
			object);

      btk_container_add (BTK_CONTAINER (prop_edit), label);
      btk_container_add (BTK_CONTAINER (prop_edit), button);
      
      g_object_connect_property (object, spec,
				 G_CALLBACK (object_changed),
				 prop_edit, B_OBJECT (label));
    }
  else if (type == B_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == BDK_TYPE_COLOR)
    {
      prop_edit = btk_color_button_new ();

      g_object_connect_property (object, spec,
				 G_CALLBACK (color_changed),
				 prop_edit, B_OBJECT (prop_edit));

      if (can_modify)
	connect_controller (B_OBJECT (prop_edit), "color-set",
			    object, spec, G_CALLBACK (color_modified));
    }
  else
    {  
      msg = g_strdup_printf ("uneditable property type: %s",
			     g_type_name (G_PARAM_SPEC_TYPE (spec)));
      prop_edit = btk_label_new (msg);            
      g_free (msg);
      btk_misc_set_alignment (BTK_MISC (prop_edit), 0.0, 0.5);
    }
  
  return prop_edit;
}

static BtkWidget *
properties_from_type (BObject *object,
		      GType    type)
{
  BtkWidget *prop_edit;
  BtkWidget *label;
  BtkWidget *sw;
  BtkWidget *vbox;
  BtkWidget *table;
  BParamSpec **specs;
  buint n_specs;
  int i;

  if (B_TYPE_IS_INTERFACE (type))
    {
      bpointer vtable = g_type_default_interface_peek (type);
      specs = g_object_interface_list_properties (vtable, &n_specs);
    }
  else
    {
      BObjectClass *class = B_OBJECT_CLASS (g_type_class_peek (type));
      specs = g_object_class_list_properties (class, &n_specs);
    }
        
  if (n_specs == 0) {
    g_free (specs);
    return NULL;
  }
  
  table = btk_table_new (n_specs, 2, FALSE);
  btk_table_set_col_spacing (BTK_TABLE (table), 0, 10);
  btk_table_set_row_spacings (BTK_TABLE (table), 3);

  i = 0;
  while (i < n_specs)
    {
      BParamSpec *spec = specs[i];
      bboolean can_modify;
      
      prop_edit = NULL;

      can_modify = ((spec->flags & G_PARAM_WRITABLE) != 0 &&
                    (spec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);
      
      if ((spec->flags & G_PARAM_READABLE) == 0)
        {
          /* can't display unreadable properties */
          ++i;
          continue;
        }
      
      if (spec->owner_type != type)
	{
	  /* we're only interested in params of type */
	  ++i;
	  continue;
	}

      label = btk_label_new (g_param_spec_get_nick (spec));
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, i, i + 1);
      
      prop_edit = property_widget (object, spec, can_modify);
      btk_table_attach_defaults (BTK_TABLE (table), prop_edit, 1, 2, i, i + 1);

      if (prop_edit)
        {
          if (!can_modify)
            btk_widget_set_sensitive (prop_edit, FALSE);

	  if (g_param_spec_get_blurb (spec))
	    btk_widget_set_tooltip_text (prop_edit, g_param_spec_get_blurb (spec));

          /* set initial value */
          g_object_notify (object, spec->name);
        }
      
      ++i;
    }


  vbox = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), table, FALSE, FALSE, 0);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (sw), vbox);

  g_free (specs);

  return sw;
}

static BtkWidget *
child_properties_from_object (BObject *object)
{
  BtkWidget *prop_edit;
  BtkWidget *label;
  BtkWidget *sw;
  BtkWidget *vbox;
  BtkWidget *table;
  BtkWidget *parent;
  BParamSpec **specs;
  buint n_specs;
  bint i;

  if (!BTK_IS_WIDGET (object))
    return NULL;

  parent = btk_widget_get_parent (BTK_WIDGET (object));

  if (!parent)
    return NULL;

  specs = btk_container_class_list_child_properties (B_OBJECT_GET_CLASS (parent), &n_specs);

  table = btk_table_new (n_specs, 2, FALSE);
  btk_table_set_col_spacing (BTK_TABLE (table), 0, 10);
  btk_table_set_row_spacings (BTK_TABLE (table), 3);

  i = 0;
  while (i < n_specs)
    {
      BParamSpec *spec = specs[i];
      bboolean can_modify;
      
      prop_edit = NULL;

      can_modify = ((spec->flags & G_PARAM_WRITABLE) != 0 &&
                    (spec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);
      
      if ((spec->flags & G_PARAM_READABLE) == 0)
        {
          /* can't display unreadable properties */
          ++i;
          continue;
        }
      
      label = btk_label_new (g_param_spec_get_nick (spec));
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, i, i + 1);
      
      mark_child_property (spec);
      prop_edit = property_widget (object, spec, can_modify);
      btk_table_attach_defaults (BTK_TABLE (table), prop_edit, 1, 2, i, i + 1);

      if (prop_edit)
        {
          if (!can_modify)
            btk_widget_set_sensitive (prop_edit, FALSE);

	  if (g_param_spec_get_blurb (spec))
	    btk_widget_set_tooltip_text (prop_edit, g_param_spec_get_blurb (spec));

          /* set initial value */
          btk_widget_child_notify (BTK_WIDGET (object), spec->name);
        }
      
      ++i;
    }

  vbox = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), table, FALSE, FALSE, 0);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (sw), vbox);

  g_free (specs);

  return sw;
}

static void
child_properties (BtkWidget *button, 
		  BObject   *object)
{
  create_prop_editor (object, 0);
}

static BtkWidget *
children_from_object (BObject *object)
{
  GList *children, *c;
  BtkWidget *table, *label, *prop_edit, *button, *vbox, *sw;
  bchar *str;
  bint i;

  if (!BTK_IS_CONTAINER (object))
    return NULL;

  children = btk_container_get_children (BTK_CONTAINER (object));

  table = btk_table_new (g_list_length (children), 2, FALSE);
  btk_table_set_col_spacing (BTK_TABLE (table), 0, 10);
  btk_table_set_row_spacings (BTK_TABLE (table), 3);
 
  for (c = children, i = 0; c; c = c->next, i++)
    {
      object = c->data;

      label = btk_label_new ("Child");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, i, i + 1);

      prop_edit = btk_hbox_new (FALSE, 5);

      str = object_label (object, NULL);
      label = btk_label_new (str);
      g_free (str);
      button = btk_button_new_with_label ("Properties");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (child_properties),
                        object);

      btk_container_add (BTK_CONTAINER (prop_edit), label);
      btk_container_add (BTK_CONTAINER (prop_edit), button);

      btk_table_attach_defaults (BTK_TABLE (table), prop_edit, 1, 2, i, i + 1);
    }

  vbox = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), table, FALSE, FALSE, 0);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (sw), vbox);

  g_list_free (children);

  return sw;
}

static BtkWidget *
cells_from_object (BObject *object)
{
  GList *cells, *c;
  BtkWidget *table, *label, *prop_edit, *button, *vbox, *sw;
  bchar *str;
  bint i;

  if (!BTK_IS_CELL_LAYOUT (object))
    return NULL;

  cells = btk_cell_layout_get_cells (BTK_CELL_LAYOUT (object));

  table = btk_table_new (g_list_length (cells), 2, FALSE);
  btk_table_set_col_spacing (BTK_TABLE (table), 0, 10);
  btk_table_set_row_spacings (BTK_TABLE (table), 3);
 
  for (c = cells, i = 0; c; c = c->next, i++)
    {
      object = c->data;

      label = btk_label_new ("Cell");
      btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
      btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, i, i + 1);

      prop_edit = btk_hbox_new (FALSE, 5);

      str = object_label (object, NULL);
      label = btk_label_new (str);
      g_free (str);
      button = btk_button_new_with_label ("Properties");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (child_properties),
                        object);

      btk_container_add (BTK_CONTAINER (prop_edit), label);
      btk_container_add (BTK_CONTAINER (prop_edit), button);

      btk_table_attach_defaults (BTK_TABLE (table), prop_edit, 1, 2, i, i + 1);
    }

  vbox = btk_vbox_new (FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), table, FALSE, FALSE, 0);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_NEVER, BTK_POLICY_AUTOMATIC);
  
  btk_scrolled_window_add_with_viewport (BTK_SCROLLED_WINDOW (sw), vbox);

  g_list_free (cells);

  return sw;
}

/* Pass zero for type if you want all properties */
BtkWidget*
create_prop_editor (BObject   *object,
		    GType      type)
{
  BtkWidget *win;
  BtkWidget *notebook;
  BtkWidget *properties;
  BtkWidget *label;
  bchar *title;
  GType *ifaces;
  buint n_ifaces;
  
  if ((win = g_object_get_data (B_OBJECT (object), "prop-editor-win")))
    {
      btk_window_present (BTK_WINDOW (win));
      return win;
    }

  win = btk_window_new (BTK_WINDOW_TOPLEVEL);
  if (BTK_IS_WIDGET (object))
    btk_window_set_screen (BTK_WINDOW (win),
			   btk_widget_get_screen (BTK_WIDGET (object)));

  /* hold a weak ref to the object we're editing */
  g_object_set_data_full (B_OBJECT (object), "prop-editor-win", win, model_destroy);
  g_object_set_data_full (B_OBJECT (win), "model-object", object, window_destroy);

  if (type == 0)
    {
      notebook = btk_notebook_new ();
      btk_notebook_set_tab_pos (BTK_NOTEBOOK (notebook), BTK_POS_LEFT);
      
      btk_container_add (BTK_CONTAINER (win), notebook);
      
      type = B_TYPE_FROM_INSTANCE (object);

      title = g_strdup_printf ("Properties of %s widget", g_type_name (type));
      btk_window_set_title (BTK_WINDOW (win), title);
      g_free (title);
      
      while (type)
	{
	  properties = properties_from_type (object, type);
	  if (properties)
	    {
	      label = btk_label_new (g_type_name (type));
	      btk_notebook_append_page (BTK_NOTEBOOK (notebook),
					properties, label);
	    }
	  
	  type = g_type_parent (type);
	}

      ifaces = g_type_interfaces (B_TYPE_FROM_INSTANCE (object), &n_ifaces);
      while (n_ifaces--)
	{
	  properties = properties_from_type (object, ifaces[n_ifaces]);
	  if (properties)
	    {
	      label = btk_label_new (g_type_name (ifaces[n_ifaces]));
	      btk_notebook_append_page (BTK_NOTEBOOK (notebook),
					properties, label);
	    }
	}

      g_free (ifaces);

      properties = child_properties_from_object (object);
      if (properties)
	{
	  label = btk_label_new ("Child properties");
	  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
				    properties, label);
	}

      properties = children_from_object (object);
      if (properties)
	{
	  label = btk_label_new ("Children");
	  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
				    properties, label);
	}

      properties = cells_from_object (object);
      if (properties)
	{
	  label = btk_label_new ("Cell renderers");
	  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
				    properties, label);
	}
    }
  else
    {
      properties = properties_from_type (object, type);
      btk_container_add (BTK_CONTAINER (win), properties);
      title = g_strdup_printf ("Properties of %s", g_type_name (type));
      btk_window_set_title (BTK_WINDOW (win), title);
      g_free (title);
    }
  
  btk_window_set_default_size (BTK_WINDOW (win), -1, 400);
  
  btk_widget_show_all (win);

  return win;
}

