/* btkcellrendereraccel.h
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
#include "btkintl.h"
#include "btkaccelgroup.h"
#include "btkmarshalers.h"
#include "btkcellrendereraccel.h"
#include "btklabel.h"
#include "btkeventbox.h"
#include "btkmain.h"
#include "btkprivate.h"
#include "bdk/bdkkeysyms.h"
#include "btkalias.h"


static void btk_cell_renderer_accel_get_property (BObject         *object,
                                                  guint            param_id,
                                                  BValue          *value,
                                                  BParamSpec      *pspec);
static void btk_cell_renderer_accel_set_property (BObject         *object,
                                                  guint            param_id,
                                                  const BValue    *value,
                                                  BParamSpec      *pspec);
static void btk_cell_renderer_accel_get_size     (BtkCellRenderer *cell,
                                                  BtkWidget       *widget,
                                                  BdkRectangle    *cell_area,
                                                  gint            *x_offset,
                                                  gint            *y_offset,
                                                  gint            *width,
                                                  gint            *height);
static BtkCellEditable *
           btk_cell_renderer_accel_start_editing (BtkCellRenderer *cell,
                                                  BdkEvent        *event,
                                                  BtkWidget       *widget,
                                                  const gchar     *path,
                                                  BdkRectangle    *background_area,
                                                  BdkRectangle    *cell_area,
                                                  BtkCellRendererState flags);
static gchar *convert_keysym_state_to_string     (BtkCellRendererAccel *accel,
                                                  guint                 keysym,
                                                  BdkModifierType       mask,
                                                  guint                 keycode);

enum {
  ACCEL_EDITED,
  ACCEL_CLEARED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ACCEL_KEY,
  PROP_ACCEL_MODS,
  PROP_KEYCODE,
  PROP_ACCEL_MODE
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkCellRendererAccel, btk_cell_renderer_accel, BTK_TYPE_CELL_RENDERER_TEXT)

static void
btk_cell_renderer_accel_init (BtkCellRendererAccel *cell_accel)
{
  gchar *text;

  text = convert_keysym_state_to_string (cell_accel, 0, 0, 0);
  g_object_set (cell_accel, "text", text, NULL);
  g_free (text);
}

static void
btk_cell_renderer_accel_class_init (BtkCellRendererAccelClass *cell_accel_class)
{
  BObjectClass *object_class;
  BtkCellRendererClass *cell_renderer_class;

  object_class = B_OBJECT_CLASS (cell_accel_class);
  cell_renderer_class = BTK_CELL_RENDERER_CLASS (cell_accel_class);

  object_class->set_property = btk_cell_renderer_accel_set_property;
  object_class->get_property = btk_cell_renderer_accel_get_property;

  cell_renderer_class->get_size      = btk_cell_renderer_accel_get_size;
  cell_renderer_class->start_editing = btk_cell_renderer_accel_start_editing;

  /**
   * BtkCellRendererAccel:accel-key:
   *
   * The keyval of the accelerator.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCEL_KEY,
                                   g_param_spec_uint ("accel-key",
                                                     P_("Accelerator key"),
                                                     P_("The keyval of the accelerator"),
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      BTK_PARAM_READWRITE));
  
  /**
   * BtkCellRendererAccel:accel-mods:
   *
   * The modifier mask of the accelerator.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCEL_MODS,
                                   g_param_spec_flags ("accel-mods",
                                                       P_("Accelerator modifiers"),
                                                       P_("The modifier mask of the accelerator"),
                                                       BDK_TYPE_MODIFIER_TYPE,
                                                       0,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererAccel:keycode:
   *
   * The hardware keycode of the accelerator. Note that the hardware keycode is
   * only relevant if the key does not have a keyval. Normally, the keyboard
   * configuration should assign keyvals to all keys.
   *
   * Since: 2.10
   */ 
  g_object_class_install_property (object_class,
                                   PROP_KEYCODE,
                                   g_param_spec_uint ("keycode",
                                                      P_("Accelerator keycode"),
                                                      P_("The hardware keycode of the accelerator"),
                                                      0,
                                                      G_MAXINT,
                                                      0,
                                                      BTK_PARAM_READWRITE));

  /**
   * BtkCellRendererAccel:accel-mode:
   *
   * Determines if the edited accelerators are BTK+ accelerators. If
   * they are, consumed modifiers are suppressed, only accelerators
   * accepted by BTK+ are allowed, and the accelerators are rendered
   * in the same way as they are in menus.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class,
                                   PROP_ACCEL_MODE,
                                   g_param_spec_enum ("accel-mode",
                                                      P_("Accelerator Mode"),
                                                      P_("The type of accelerators"),
                                                      BTK_TYPE_CELL_RENDERER_ACCEL_MODE,
                                                      BTK_CELL_RENDERER_ACCEL_MODE_BTK,
                                                      BTK_PARAM_READWRITE));
  
  /**
   * BtkCellRendererAccel::accel-edited:
   * @accel: the object reveiving the signal
   * @path_string: the path identifying the row of the edited cell
   * @accel_key: the new accelerator keyval
   * @accel_mods: the new acclerator modifier mask
   * @hardware_keycode: the keycode of the new accelerator
   *
   * Gets emitted when the user has selected a new accelerator.
   *
   * Since: 2.10
   */
  signals[ACCEL_EDITED] = g_signal_new (I_("accel-edited"),
                                        BTK_TYPE_CELL_RENDERER_ACCEL,
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (BtkCellRendererAccelClass, accel_edited),
                                        NULL, NULL,
                                        _btk_marshal_VOID__STRING_UINT_FLAGS_UINT,
                                        B_TYPE_NONE, 4,
                                        B_TYPE_STRING,
                                        B_TYPE_UINT,
                                        BDK_TYPE_MODIFIER_TYPE,
                                        B_TYPE_UINT);

  /**
   * BtkCellRendererAccel::accel-cleared:
   * @accel: the object reveiving the signal
   * @path_string: the path identifying the row of the edited cell
   *
   * Gets emitted when the user has removed the accelerator.
   *
   * Since: 2.10
   */
  signals[ACCEL_CLEARED] = g_signal_new (I_("accel-cleared"),
                                         BTK_TYPE_CELL_RENDERER_ACCEL,
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (BtkCellRendererAccelClass, accel_cleared),
                                         NULL, NULL,
                                         g_cclosure_marshal_VOID__STRING,
                                         B_TYPE_NONE, 1,
                                         B_TYPE_STRING);
}


/**
 * btk_cell_renderer_accel_new:
 *
 * Creates a new #BtkCellRendererAccel.
 * 
 * Returns: the new cell renderer
 *
 * Since: 2.10
 */
BtkCellRenderer *
btk_cell_renderer_accel_new (void)
{
  return g_object_new (BTK_TYPE_CELL_RENDERER_ACCEL, NULL);
}

static gchar *
convert_keysym_state_to_string (BtkCellRendererAccel *accel,
                                guint                 keysym,
                                BdkModifierType       mask,
                                guint                 keycode)
{
  if (keysym == 0 && keycode == 0)
    /* This label is displayed in a treeview cell displaying
     * a disabled accelerator key combination.
     */
    return g_strdup (C_("Accelerator", "Disabled"));
  else 
    {
      if (accel->accel_mode == BTK_CELL_RENDERER_ACCEL_MODE_BTK)
        {
          if (!btk_accelerator_valid (keysym, mask))
            /* This label is displayed in a treeview cell displaying
             * an accelerator key combination that is not valid according
             * to btk_accelerator_valid().
             */
            return g_strdup (C_("Accelerator", "Invalid"));

          return btk_accelerator_get_label (keysym, mask);
        }
      else 
        {
          gchar *name;

          name = btk_accelerator_get_label (keysym, mask);
          if (name == NULL)
            name = btk_accelerator_name (keysym, mask);

          if (keysym == 0)
            {
              gchar *tmp;

              tmp = name;
              name = g_strdup_printf ("%s0x%02x", tmp, keycode);
              g_free (tmp);
            }

          return name;
        }
    }
}

static void
btk_cell_renderer_accel_get_property  (BObject    *object,
                                       guint       param_id,
                                       BValue     *value,
                                       BParamSpec *pspec)
{
  BtkCellRendererAccel *accel = BTK_CELL_RENDERER_ACCEL (object);

  switch (param_id)
    {
    case PROP_ACCEL_KEY:
      b_value_set_uint (value, accel->accel_key);
      break;

    case PROP_ACCEL_MODS:
      b_value_set_flags (value, accel->accel_mods);
      break;

    case PROP_KEYCODE:
      b_value_set_uint (value, accel->keycode);
      break;

    case PROP_ACCEL_MODE:
      b_value_set_enum (value, accel->accel_mode);
      break;

    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
btk_cell_renderer_accel_set_property  (BObject      *object,
                                       guint         param_id,
                                       const BValue *value,
                                       BParamSpec   *pspec)
{
  BtkCellRendererAccel *accel = BTK_CELL_RENDERER_ACCEL (object);
  gboolean changed = FALSE;

  switch (param_id)
    {
    case PROP_ACCEL_KEY:
      {
        guint accel_key = b_value_get_uint (value);

        if (accel->accel_key != accel_key)
          {
            accel->accel_key = accel_key;
            changed = TRUE;
          }
      }
      break;

    case PROP_ACCEL_MODS:
      {
        guint accel_mods = b_value_get_flags (value);

        if (accel->accel_mods != accel_mods)
          {
            accel->accel_mods = accel_mods;
            changed = TRUE;
          }
      }
      break;
    case PROP_KEYCODE:
      {
        guint keycode = b_value_get_uint (value);

        if (accel->keycode != keycode)
          {
            accel->keycode = keycode;
            changed = TRUE;
          }
      }
      break;

    case PROP_ACCEL_MODE:
      accel->accel_mode = b_value_get_enum (value);
      break;
      
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }

  if (changed)
    {
      gchar *text;

      text = convert_keysym_state_to_string (accel, accel->accel_key, accel->accel_mods, accel->keycode);
      g_object_set (accel, "text", text, NULL);
      g_free (text);
    }
}

static void
btk_cell_renderer_accel_get_size (BtkCellRenderer *cell,
                                  BtkWidget       *widget,
                                  BdkRectangle    *cell_area,
                                  gint            *x_offset,
                                  gint            *y_offset,
                                  gint            *width,
                                  gint            *height)

{
  BtkCellRendererAccel *accel = (BtkCellRendererAccel *) cell;
  BtkRequisition requisition;

  if (accel->sizing_label == NULL)
    accel->sizing_label = btk_label_new (_("New accelerator..."));

  btk_widget_size_request (accel->sizing_label, &requisition);

  BTK_CELL_RENDERER_CLASS (btk_cell_renderer_accel_parent_class)->get_size (cell, widget, cell_area,
                                                                            x_offset, y_offset, width, height);

  /* FIXME: need to take the cell_area et al. into account */
  if (width)
    *width = MAX (*width, requisition.width);
  if (height)
    *height = MAX (*height, requisition.height);
}

static gboolean
grab_key_callback (BtkWidget            *widget,
                   BdkEventKey          *event,
                   BtkCellRendererAccel *accel)
{
  BdkModifierType accel_mods = 0;
  guint accel_key;
  guint keyval;
  gchar *path;
  gboolean edited;
  gboolean cleared;
  BdkModifierType consumed_modifiers;
  BdkDisplay *display;

  display = btk_widget_get_display (widget);

  if (event->is_modifier)
    return TRUE;

  edited = FALSE;
  cleared = FALSE;

  accel_mods = event->state;

  _btk_translate_keyboard_accel_state (bdk_keymap_get_for_display (display),
                                       event->hardware_keycode,
                                       event->state,
                                       btk_accelerator_get_default_mod_mask (),
                                       event->group,
                                       &keyval, NULL, NULL, &consumed_modifiers);

  accel_key = bdk_keyval_to_lower (keyval);
  if (accel_key == BDK_ISO_Left_Tab) 
    accel_key = BDK_Tab;

  accel_mods &= btk_accelerator_get_default_mod_mask ();

  /* Filter consumed modifiers 
   */
  if (accel->accel_mode == BTK_CELL_RENDERER_ACCEL_MODE_BTK)
    accel_mods &= ~consumed_modifiers;
  
  /* Put shift back if it changed the case of the key, not otherwise.
   */
  if (accel_key != keyval)
    accel_mods |= BDK_SHIFT_MASK;
    
  if (accel_mods == 0)
    {
      switch (keyval)
	{
	case BDK_Escape:
	  goto out; /* cancel */
	case BDK_BackSpace:
	  /* clear the accelerator on Backspace */
	  cleared = TRUE;
	  goto out;
	default:
	  break;
	}
    }

  if (accel->accel_mode == BTK_CELL_RENDERER_ACCEL_MODE_BTK)
    {
      if (!btk_accelerator_valid (accel_key, accel_mods))
        {
          btk_widget_error_bell (widget);

          return TRUE;
        }
    }

  edited = TRUE;

 out:
  btk_grab_remove (accel->grab_widget);
  bdk_display_keyboard_ungrab (display, event->time);
  bdk_display_pointer_ungrab (display, event->time);

  path = g_strdup (g_object_get_data (B_OBJECT (accel->edit_widget), "btk-cell-renderer-text"));

  btk_cell_editable_editing_done (BTK_CELL_EDITABLE (accel->edit_widget));
  btk_cell_editable_remove_widget (BTK_CELL_EDITABLE (accel->edit_widget));
  accel->edit_widget = NULL;
  accel->grab_widget = NULL;
  
  if (edited)
    g_signal_emit (accel, signals[ACCEL_EDITED], 0, path, 
                   accel_key, accel_mods, event->hardware_keycode);
  else if (cleared)
    g_signal_emit (accel, signals[ACCEL_CLEARED], 0, path);

  g_free (path);

  return TRUE;
}

static void
ungrab_stuff (BtkWidget            *widget,
              BtkCellRendererAccel *accel)
{
  BdkDisplay *display = btk_widget_get_display (widget);

  btk_grab_remove (accel->grab_widget);
  bdk_display_keyboard_ungrab (display, BDK_CURRENT_TIME);
  bdk_display_pointer_ungrab (display, BDK_CURRENT_TIME);

  g_signal_handlers_disconnect_by_func (B_OBJECT (accel->grab_widget),
                                        G_CALLBACK (grab_key_callback),
                                        accel);
}

static void
_btk_cell_editable_event_box_start_editing (BtkCellEditable *cell_editable,
                                            BdkEvent        *event)
{
  /* do nothing, because we are pointless */
}

static void
_btk_cell_editable_event_box_cell_editable_init (BtkCellEditableIface *iface)
{
  iface->start_editing = _btk_cell_editable_event_box_start_editing;
}

typedef struct _BtkCellEditableEventBox BtkCellEditableEventBox;
typedef         BtkEventBoxClass        BtkCellEditableEventBoxClass;

struct _BtkCellEditableEventBox
{
  BtkEventBox box;
  gboolean editing_canceled;
};

G_DEFINE_TYPE_WITH_CODE (BtkCellEditableEventBox, _btk_cell_editable_event_box, BTK_TYPE_EVENT_BOX, { \
    G_IMPLEMENT_INTERFACE (BTK_TYPE_CELL_EDITABLE, _btk_cell_editable_event_box_cell_editable_init)   \
      })

enum {
  PROP_ZERO,
  PROP_EDITING_CANCELED
};

static void
btk_cell_editable_event_box_set_property (BObject      *object,
                                          guint         prop_id,
                                          const BValue *value,
                                          BParamSpec   *pspec)
{
  BtkCellEditableEventBox *box = (BtkCellEditableEventBox*)object;

  switch (prop_id)
    {
    case PROP_EDITING_CANCELED:
      box->editing_canceled = b_value_get_boolean (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_cell_editable_event_box_get_property (BObject    *object,
                                          guint       prop_id,
                                          BValue     *value,
                                          BParamSpec *pspec)
{
  BtkCellEditableEventBox *box = (BtkCellEditableEventBox*)object;

  switch (prop_id)
    {
    case PROP_EDITING_CANCELED:
      b_value_set_boolean (value, box->editing_canceled);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
_btk_cell_editable_event_box_class_init (BtkCellEditableEventBoxClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  bobject_class->set_property = btk_cell_editable_event_box_set_property;
  bobject_class->get_property = btk_cell_editable_event_box_get_property;

  g_object_class_override_property (bobject_class,
                                    PROP_EDITING_CANCELED,
                                    "editing-canceled");
}

static void
_btk_cell_editable_event_box_init (BtkCellEditableEventBox *box)
{
}

static BtkCellEditable *
btk_cell_renderer_accel_start_editing (BtkCellRenderer      *cell,
                                       BdkEvent             *event,
                                       BtkWidget            *widget,
                                       const gchar          *path,
                                       BdkRectangle         *background_area,
                                       BdkRectangle         *cell_area,
                                       BtkCellRendererState  flags)
{
  BtkCellRendererText *celltext;
  BtkCellRendererAccel *accel;
  BtkWidget *label;
  BtkWidget *eventbox;
  
  celltext = BTK_CELL_RENDERER_TEXT (cell);
  accel = BTK_CELL_RENDERER_ACCEL (cell);

  /* If the cell isn't editable we return NULL. */
  if (celltext->editable == FALSE)
    return NULL;

  g_return_val_if_fail (widget->window != NULL, NULL);
  
  if (bdk_keyboard_grab (widget->window, FALSE,
                         bdk_event_get_time (event)) != BDK_GRAB_SUCCESS)
    return NULL;

  if (bdk_pointer_grab (widget->window, FALSE,
                        BDK_BUTTON_PRESS_MASK,
                        NULL, NULL,
                        bdk_event_get_time (event)) != BDK_GRAB_SUCCESS)
    {
      bdk_display_keyboard_ungrab (btk_widget_get_display (widget),
                                   bdk_event_get_time (event));
      return NULL;
    }
  
  accel->grab_widget = widget;

  g_signal_connect (B_OBJECT (widget), "key-press-event",
                    G_CALLBACK (grab_key_callback),
                    accel);

  eventbox = g_object_new (_btk_cell_editable_event_box_get_type (), NULL);
  accel->edit_widget = eventbox;
  g_object_add_weak_pointer (B_OBJECT (accel->edit_widget),
                             (gpointer) &accel->edit_widget);
  
  label = btk_label_new (NULL);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  
  btk_widget_modify_bg (eventbox, BTK_STATE_NORMAL,
                        &widget->style->bg[BTK_STATE_SELECTED]);

  btk_widget_modify_fg (label, BTK_STATE_NORMAL,
                        &widget->style->fg[BTK_STATE_SELECTED]);
  
  /* This label is displayed in a treeview cell displaying
   * an accelerator when the cell is clicked to change the 
   * acelerator.
   */
  btk_label_set_text (BTK_LABEL (label), _("New accelerator..."));

  btk_container_add (BTK_CONTAINER (eventbox), label);
  
  g_object_set_data_full (B_OBJECT (accel->edit_widget), "btk-cell-renderer-text",
                          g_strdup (path), g_free);
  
  btk_widget_show_all (accel->edit_widget);

  btk_grab_add (accel->grab_widget);

  g_signal_connect (B_OBJECT (accel->edit_widget), "unrealize",
                    G_CALLBACK (ungrab_stuff), accel);
  
  return BTK_CELL_EDITABLE (accel->edit_widget);
}


#define __BTK_CELL_RENDERER_ACCEL_C__
#include "btkaliasdef.c"
