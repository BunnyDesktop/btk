/* BAIL - The BUNNY Accessibility Enabling Library
 * Copyright 2001 Sun Microsystems Inc.
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
#include <btk/btk.h>
#include "bailtextcell.h"
#include "bailcontainercell.h"
#include "bailcellparent.h"
#include <libbail-util/bailmisc.h>
#include "bail-private-macros.h"

static void      bail_text_cell_class_init		(BailTextCellClass *klass);
static void      bail_text_cell_init			(BailTextCell	*text_cell);
static void      bail_text_cell_finalize		(BObject	*object);

static const gchar* bail_text_cell_get_name    (BatkObject      *batk_obj);

static void      batk_text_interface_init		(BatkTextIface	*iface);

/* batktext.h */

static gchar*    bail_text_cell_get_text		(BatkText	*text,
							gint		start_pos,
							gint		end_pos);
static gunichar bail_text_cell_get_character_at_offset	(BatkText	*text,
							 gint		offset);
static gchar*	bail_text_cell_get_text_before_offset	(BatkText	*text,
							 gint		offset,
							 BatkTextBoundary boundary_type,
							 gint		*start_offset,
							 gint		*end_offset);
static gchar*	bail_text_cell_get_text_at_offset	(BatkText	*text,
							 gint		offset,
							 BatkTextBoundary boundary_type,
							 gint		*start_offset,
							 gint		*end_offset);
static gchar*	bail_text_cell_get_text_after_offset	(BatkText	*text,
							 gint		offset,
							 BatkTextBoundary boundary_type,
							 gint		*start_offset,
							 gint		*end_offset);
static gint      bail_text_cell_get_character_count	(BatkText	*text);
static gint      bail_text_cell_get_caret_offset	(BatkText	*text);
static gboolean  bail_text_cell_set_caret_offset	(BatkText	*text,
							 gint		offset);
static void      bail_text_cell_get_character_extents	(BatkText	*text,
							 gint		offset,
							 gint		*x,
							 gint		*y,
							 gint		*width,
							 gint		*height,
							 BatkCoordType	coords);
static gint      bail_text_cell_get_offset_at_point	(BatkText	*text,
							 gint		x,
							 gint		y,
							 BatkCoordType	coords);
static BatkAttributeSet* bail_text_cell_get_run_attributes 
                                                        (BatkText	*text,
							 gint		offset,
							 gint		*start_offset,      
							 gint		*end_offset); 
static BatkAttributeSet* bail_text_cell_get_default_attributes 
                                                        (BatkText        *text);

static BangoLayout*     create_bango_layout             (BtkCellRendererText *btk_renderer,
                                                         BtkWidget           *widget);
static void             add_attr                        (BangoAttrList  *attr_list,
                                                         BangoAttribute *attr);

/* Misc */

static gboolean bail_text_cell_update_cache		(BailRendererCell *cell,
							 gboolean	emit_change_signal);

gchar *bail_text_cell_property_list[] = {
  /* Set font_desc first since it resets other values if it is NULL */
  "font_desc",

  "attributes",
  "background_bdk",
  "editable",
  "family",
  "foreground_bdk",
  "rise",
  "scale",
  "size",
  "size_points",
  "stretch",
  "strikethrough",
  "style",
  "text",
  "underline",
  "variant",
  "weight",

  /* Also need the sets */
  "background_set",
  "editable_set",
  "family_set",
  "foreground_set",
  "rise_set",
  "scale_set",
  "size_set",
  "stretch_set",
  "strikethrough_set",
  "style_set",
  "underline_set",
  "variant_set",
  "weight_set",
  NULL
};

G_DEFINE_TYPE_WITH_CODE (BailTextCell, bail_text_cell, BAIL_TYPE_RENDERER_CELL,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void 
bail_text_cell_class_init (BailTextCellClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *batk_object_class = BATK_OBJECT_CLASS (klass);
  BailRendererCellClass *renderer_cell_class = BAIL_RENDERER_CELL_CLASS (klass);

  renderer_cell_class->update_cache = bail_text_cell_update_cache;
  renderer_cell_class->property_list = bail_text_cell_property_list;

  batk_object_class->get_name = bail_text_cell_get_name;

  bobject_class->finalize = bail_text_cell_finalize;
}

/* batktext.h */

static void
bail_text_cell_init (BailTextCell *text_cell)
{
  text_cell->cell_text = NULL;
  text_cell->caret_pos = 0;
  text_cell->cell_length = 0;
  text_cell->textutil = bail_text_util_new ();
  batk_state_set_add_state (BAIL_CELL (text_cell)->state_set,
                           BATK_STATE_SINGLE_LINE);
}

BatkObject* 
bail_text_cell_new (void)
{
  BObject *object;
  BatkObject *batk_object;
  BailRendererCell *cell;

  object = g_object_new (BAIL_TYPE_TEXT_CELL, NULL);

  g_return_val_if_fail (object != NULL, NULL);

  batk_object = BATK_OBJECT (object);
  batk_object->role = BATK_ROLE_TABLE_CELL;

  cell = BAIL_RENDERER_CELL(object);

  cell->renderer = btk_cell_renderer_text_new ();
  g_object_ref_sink (cell->renderer);
  return batk_object;
}

static void
bail_text_cell_finalize (BObject            *object)
{
  BailTextCell *text_cell = BAIL_TEXT_CELL (object);

  g_object_unref (text_cell->textutil);
  g_free (text_cell->cell_text);

  B_OBJECT_CLASS (bail_text_cell_parent_class)->finalize (object);
}

static const gchar*
bail_text_cell_get_name (BatkObject *batk_obj)
{
  if (batk_obj->name)
    return batk_obj->name;
  else
    {
      BailTextCell *text_cell = BAIL_TEXT_CELL (batk_obj);

      return text_cell->cell_text;
    }
}

static gboolean
bail_text_cell_update_cache (BailRendererCell *cell,
                             gboolean         emit_change_signal)
{
  BailTextCell *text_cell = BAIL_TEXT_CELL (cell);
  BatkObject *obj = BATK_OBJECT (cell);
  gboolean rv = FALSE;
  gint temp_length;
  gchar *new_cache;

  g_object_get (B_OBJECT (cell->renderer), "text", &new_cache, NULL);

  if (text_cell->cell_text)
    {
     /*
      * If the new value is NULL and the old value isn't NULL, then the
      * value has changed.
      */
      if (new_cache == NULL ||
          strcmp (text_cell->cell_text, new_cache))
        {
          g_free (text_cell->cell_text);
          temp_length = text_cell->cell_length;
          text_cell->cell_text = NULL;
          text_cell->cell_length = 0;
          if (emit_change_signal)
            {
              g_signal_emit_by_name (cell, "text_changed::delete", 0, temp_length);
              if (obj->name == NULL)
                g_object_notify (B_OBJECT (obj), "accessible-name");
            }
          if (new_cache)
            rv = TRUE;
        }
    }
  else
    rv = TRUE;

  if (rv)
    {
      if (new_cache == NULL)
        {
          text_cell->cell_text = g_strdup ("");
          text_cell->cell_length = 0;
        }
      else
        {
          text_cell->cell_text = g_strdup (new_cache);
          text_cell->cell_length = g_utf8_strlen (new_cache, -1);
        }
    }

  g_free (new_cache);
  bail_text_util_text_setup (text_cell->textutil, text_cell->cell_text);
  
  if (rv)
    {
      if (emit_change_signal)
        {
          g_signal_emit_by_name (cell, "text_changed::insert",
  			         0, text_cell->cell_length);

          if (obj->name == NULL)
            g_object_notify (B_OBJECT (obj), "accessible-name");
        }
    }
  return rv;
}

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_text_cell_get_text;
  iface->get_character_at_offset = bail_text_cell_get_character_at_offset;
  iface->get_text_before_offset = bail_text_cell_get_text_before_offset;
  iface->get_text_at_offset = bail_text_cell_get_text_at_offset;
  iface->get_text_after_offset = bail_text_cell_get_text_after_offset;
  iface->get_character_count = bail_text_cell_get_character_count;
  iface->get_caret_offset = bail_text_cell_get_caret_offset;
  iface->set_caret_offset = bail_text_cell_set_caret_offset;
  iface->get_run_attributes = bail_text_cell_get_run_attributes;
  iface->get_default_attributes = bail_text_cell_get_default_attributes;
  iface->get_character_extents = bail_text_cell_get_character_extents;
  iface->get_offset_at_point = bail_text_cell_get_offset_at_point;
}

static gchar* 
bail_text_cell_get_text (BatkText *text, 
                         gint    start_pos,
                         gint    end_pos)
{
  if (BAIL_TEXT_CELL (text)->cell_text)
    return bail_text_util_get_substring (BAIL_TEXT_CELL (text)->textutil,
              start_pos, end_pos);
  else
    return g_strdup ("");
}

static gchar* 
bail_text_cell_get_text_before_offset (BatkText         *text,
                                       gint            offset,
                                       BatkTextBoundary boundary_type,
                                       gint            *start_offset,
                                       gint            *end_offset)
{
  return bail_text_util_get_text (BAIL_TEXT_CELL (text)->textutil,
        NULL, BAIL_BEFORE_OFFSET, boundary_type, offset, start_offset,
        end_offset);
}

static gchar* 
bail_text_cell_get_text_at_offset (BatkText         *text,
                                   gint            offset,
                                   BatkTextBoundary boundary_type,
                                   gint            *start_offset,
                                   gint            *end_offset)
{
  return bail_text_util_get_text (BAIL_TEXT_CELL (text)->textutil,
        NULL, BAIL_AT_OFFSET, boundary_type, offset, start_offset, end_offset);
}

static gchar* 
bail_text_cell_get_text_after_offset (BatkText         *text,
                                      gint            offset,
                                      BatkTextBoundary boundary_type,
                                      gint            *start_offset,
                                      gint            *end_offset)
{
  return bail_text_util_get_text (BAIL_TEXT_CELL (text)->textutil,
        NULL, BAIL_AFTER_OFFSET, boundary_type, offset, start_offset,
        end_offset);
}

static gint 
bail_text_cell_get_character_count (BatkText *text)
{
  if (BAIL_TEXT_CELL (text)->cell_text != NULL)
    return BAIL_TEXT_CELL (text)->cell_length;
  else
    return 0;
}

static gint 
bail_text_cell_get_caret_offset (BatkText *text)
{
  return BAIL_TEXT_CELL (text)->caret_pos;
}

static gboolean 
bail_text_cell_set_caret_offset (BatkText *text,
                                 gint    offset)
{
  BailTextCell *text_cell = BAIL_TEXT_CELL (text);

  if (text_cell->cell_text == NULL)
    return FALSE;
  else
    {

      /* Only set the caret within the bounds and if it is to a new position. */
      if (offset >= 0 && 
          offset <= text_cell->cell_length &&
          offset != text_cell->caret_pos)
        {
          text_cell->caret_pos = offset;

          /* emit the signal */
          g_signal_emit_by_name (text, "text_caret_moved", offset);
          return TRUE;
        }
      else
        return FALSE;
    }
}

static BatkAttributeSet*
bail_text_cell_get_run_attributes (BatkText *text,
                                  gint     offset,
                                  gint     *start_offset,
                                  gint     *end_offset) 
{
  BailRendererCell *bail_renderer; 
  BtkCellRendererText *btk_renderer;
  BatkAttributeSet *attrib_set = NULL;
  BangoLayout *layout;
  BatkObject *parent;
  BtkWidget *widget;

  bail_renderer = BAIL_RENDERER_CELL (text);
  btk_renderer = BTK_CELL_RENDERER_TEXT (bail_renderer->renderer);

  parent = batk_object_get_parent (BATK_OBJECT (text));
  if (BAIL_IS_CONTAINER_CELL (parent))
    parent = batk_object_get_parent (parent);
  g_return_val_if_fail (BAIL_IS_CELL_PARENT (parent), NULL);
  widget = BTK_ACCESSIBLE (parent)->widget;
  layout = create_bango_layout (btk_renderer, widget),
  attrib_set = bail_misc_layout_get_run_attributes (attrib_set, 
                                                    layout,
                                                    btk_renderer->text,
                                                    offset,
                                                    start_offset,
                                                    end_offset);
  g_object_unref (B_OBJECT (layout));
  
  return attrib_set;
}

static BatkAttributeSet*
bail_text_cell_get_default_attributes (BatkText	*text)
{
  BailRendererCell *bail_renderer; 
  BtkCellRendererText *btk_renderer;
  BatkAttributeSet *attrib_set = NULL;
  BangoLayout *layout;
  BatkObject *parent;
  BtkWidget *widget;

  bail_renderer = BAIL_RENDERER_CELL (text);
  btk_renderer = BTK_CELL_RENDERER_TEXT (bail_renderer->renderer);

  parent = batk_object_get_parent (BATK_OBJECT (text));
  if (BAIL_IS_CONTAINER_CELL (parent))
    parent = batk_object_get_parent (parent);
  g_return_val_if_fail (BAIL_IS_CELL_PARENT (parent), NULL);
  widget = BTK_ACCESSIBLE (parent)->widget;
  layout = create_bango_layout (btk_renderer, widget),

  attrib_set = bail_misc_get_default_attributes (attrib_set, 
                                                 layout,
                                                 widget);
  g_object_unref (B_OBJECT (layout));
  return attrib_set;
}

/* 
 * This function is used by bail_text_cell_get_offset_at_point()
 * and bail_text_cell_get_character_extents(). There is no 
 * cached BangoLayout for bailtextcell so we must create a temporary
 * one using this function.
 */ 
static BangoLayout*
create_bango_layout(BtkCellRendererText *btk_renderer,
                    BtkWidget           *widget)
{
  BangoAttrList *attr_list;
  BangoLayout *layout;
  BangoUnderline uline;
  BangoFontMask mask;

  layout = btk_widget_create_bango_layout (widget, btk_renderer->text);

  if (btk_renderer->extra_attrs)
    attr_list = bango_attr_list_copy (btk_renderer->extra_attrs);
  else
    attr_list = bango_attr_list_new ();

  if (btk_renderer->foreground_set)
    {
      BangoColor color;
      color = btk_renderer->foreground;
      add_attr (attr_list, bango_attr_foreground_new (color.red,
                                                      color.green, color.blue));
    }

  if (btk_renderer->strikethrough_set)
    add_attr (attr_list,
              bango_attr_strikethrough_new (btk_renderer->strikethrough));

  mask = bango_font_description_get_set_fields (btk_renderer->font);

  if (mask & BANGO_FONT_MASK_FAMILY)
    add_attr (attr_list,
      bango_attr_family_new (bango_font_description_get_family (btk_renderer->font)));

  if (mask & BANGO_FONT_MASK_STYLE)
    add_attr (attr_list, bango_attr_style_new (bango_font_description_get_style (btk_renderer->font)));

  if (mask & BANGO_FONT_MASK_VARIANT)
    add_attr (attr_list, bango_attr_variant_new (bango_font_description_get_variant (btk_renderer->font)));

  if (mask & BANGO_FONT_MASK_WEIGHT)
    add_attr (attr_list, bango_attr_weight_new (bango_font_description_get_weight (btk_renderer->font)));

  if (mask & BANGO_FONT_MASK_STRETCH)
    add_attr (attr_list, bango_attr_stretch_new (bango_font_description_get_stretch (btk_renderer->font)));

  if (mask & BANGO_FONT_MASK_SIZE)
    add_attr (attr_list, bango_attr_size_new (bango_font_description_get_size (btk_renderer->font)));

  if (btk_renderer->scale_set &&
      btk_renderer->font_scale != 1.0)
    add_attr (attr_list, bango_attr_scale_new (btk_renderer->font_scale));

  if (btk_renderer->underline_set)
    uline = btk_renderer->underline_style;
  else
    uline = BANGO_UNDERLINE_NONE;

  if (uline != BANGO_UNDERLINE_NONE)
    add_attr (attr_list,
      bango_attr_underline_new (btk_renderer->underline_style));

  if (btk_renderer->rise_set)
    add_attr (attr_list, bango_attr_rise_new (btk_renderer->rise));

  bango_layout_set_attributes (layout, attr_list);
  bango_layout_set_width (layout, -1);
  bango_attr_list_unref (attr_list);

  return layout;
}

static void 
add_attr (BangoAttrList  *attr_list,
         BangoAttribute *attr)
{
  attr->start_index = 0;
  attr->end_index = G_MAXINT;
  bango_attr_list_insert (attr_list, attr);
}

static void      
bail_text_cell_get_character_extents (BatkText          *text,
                                      gint             offset,
                                      gint             *x,
                                      gint             *y,
                                      gint             *width,
                                      gint             *height,
                                      BatkCoordType     coords)
{
  BailRendererCell *bail_renderer; 
  BtkCellRendererText *btk_renderer;
  BdkRectangle rendered_rect;
  BtkWidget *widget;
  BatkObject *parent;
  BangoRectangle char_rect;
  BangoLayout *layout;
  gint x_offset, y_offset, index, cell_height, cell_width;

  if (!BAIL_TEXT_CELL (text)->cell_text)
    {
      *x = *y = *height = *width = 0;
      return;
    }
  if (offset < 0 || offset >= BAIL_TEXT_CELL (text)->cell_length)
    {
      *x = *y = *height = *width = 0;
      return;
    }
  bail_renderer = BAIL_RENDERER_CELL (text);
  btk_renderer = BTK_CELL_RENDERER_TEXT (bail_renderer->renderer);
  /*
   * Thus would be inconsistent with the cache
   */
  bail_return_if_fail (btk_renderer->text);

  parent = batk_object_get_parent (BATK_OBJECT (text));
  if (BAIL_IS_CONTAINER_CELL (parent))
    parent = batk_object_get_parent (parent);
  widget = BTK_ACCESSIBLE (parent)->widget;
  g_return_if_fail (BAIL_IS_CELL_PARENT (parent));
  bail_cell_parent_get_cell_area (BAIL_CELL_PARENT (parent), BAIL_CELL (text),
                                  &rendered_rect);

  btk_cell_renderer_get_size (BTK_CELL_RENDERER (btk_renderer), widget,
    &rendered_rect, &x_offset, &y_offset, &cell_width, &cell_height);
  layout = create_bango_layout (btk_renderer, widget);

  index = g_utf8_offset_to_pointer (btk_renderer->text,
    offset) - btk_renderer->text;
  bango_layout_index_to_pos (layout, index, &char_rect); 

  bail_misc_get_extents_from_bango_rectangle (widget,
      &char_rect,
      x_offset + rendered_rect.x + bail_renderer->renderer->xpad,
      y_offset + rendered_rect.y + bail_renderer->renderer->ypad,
      x, y, width, height, coords);
  g_object_unref (layout);
  return;
} 

static gint      
bail_text_cell_get_offset_at_point (BatkText          *text,
                                    gint             x,
                                    gint             y,
                                    BatkCoordType     coords)
{
  BatkObject *parent;
  BailRendererCell *bail_renderer; 
  BtkCellRendererText *btk_renderer;
  BtkWidget *widget;
  BdkRectangle rendered_rect;
  BangoLayout *layout;
  gint x_offset, y_offset, index;
 
  if (!BAIL_TEXT_CELL (text)->cell_text)
    return -1;

  bail_renderer = BAIL_RENDERER_CELL (text);
  btk_renderer = BTK_CELL_RENDERER_TEXT (bail_renderer->renderer);
  parent = batk_object_get_parent (BATK_OBJECT (text));

  g_return_val_if_fail (btk_renderer->text, -1);
  if (BAIL_IS_CONTAINER_CELL (parent))
    parent = batk_object_get_parent (parent);

  widget = BTK_ACCESSIBLE (parent)->widget;

  g_return_val_if_fail (BAIL_IS_CELL_PARENT (parent), -1);
  bail_cell_parent_get_cell_area (BAIL_CELL_PARENT (parent), BAIL_CELL (text),
                                  &rendered_rect);
  btk_cell_renderer_get_size (BTK_CELL_RENDERER (btk_renderer), widget,
     &rendered_rect, &x_offset, &y_offset, NULL, NULL);

  layout = create_bango_layout (btk_renderer, widget);
   
  index = bail_misc_get_index_at_point_in_layout (widget, layout,
        x_offset + rendered_rect.x + bail_renderer->renderer->xpad,
        y_offset + rendered_rect.y + bail_renderer->renderer->ypad,
        x, y, coords);
  g_object_unref (layout);
  if (index == -1)
    {
      if (coords == BATK_XY_WINDOW || coords == BATK_XY_SCREEN)
        return g_utf8_strlen (btk_renderer->text, -1);
    
      return index;  
    }
  else
    return g_utf8_pointer_to_offset (btk_renderer->text,
       btk_renderer->text + index);  
}

static gunichar 
bail_text_cell_get_character_at_offset (BatkText       *text,
                                        gint          offset)
{
  gchar *index;
  gchar *string;

  string = BAIL_TEXT_CELL(text)->cell_text;

  if (!string)
    return '\0';

  if (offset >= g_utf8_strlen (string, -1))
    return '\0';

  index = g_utf8_offset_to_pointer (string, offset);

  return g_utf8_get_char (index);
}

