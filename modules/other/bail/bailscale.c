/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2004 Sun Microsystems Inc.
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
#include "bailscale.h"
#include <libbail-util/bailmisc.h>

static void	    bail_scale_class_init        (BailScaleClass *klass);

static void         bail_scale_init              (BailScale      *scale);

static void         bail_scale_real_initialize   (BatkObject      *obj,
                                                  gpointer      data);
static void         bail_scale_notify            (BObject       *obj,
                                                  BParamSpec    *pspec);
static void         bail_scale_finalize          (BObject        *object);

/* batktext.h */ 
static void	    batk_text_interface_init        (BatkTextIface      *iface);

static gchar*	    bail_scale_get_text	           (BatkText	      *text,
                                                    gint	      start_pos,
						    gint	      end_pos);
static gunichar	    bail_scale_get_character_at_offset
                                                   (BatkText	      *text,
						    gint	      offset);
static gchar*       bail_scale_get_text_before_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*       bail_scale_get_text_at_offset  (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gchar*      bail_scale_get_text_after_offset
                                                   (BatkText	      *text,
 						    gint	      offset,
						    BatkTextBoundary   boundary_type,
						    gint	      *start_offset,
						    gint	      *end_offset);
static gint	   bail_scale_get_character_count  (BatkText	      *text);
static void        bail_scale_get_character_extents
                                                   (BatkText	      *text,
						    gint 	      offset,
		                                    gint 	      *x,
                    		   	            gint 	      *y,
                                		    gint 	      *width,
                                     		    gint 	      *height,
			        		    BatkCoordType      coords);
static gint        bail_scale_get_offset_at_point  (BatkText           *text,
                                                    gint              x,
                                                    gint              y,
			                            BatkCoordType      coords);
static BatkAttributeSet* bail_scale_get_run_attributes 
                                                   (BatkText           *text,
              					    gint 	      offset,
                                                    gint 	      *start_offset,
					            gint	      *end_offset);
static BatkAttributeSet* bail_scale_get_default_attributes
                                                   (BatkText           *text);

G_DEFINE_TYPE_WITH_CODE (BailScale, bail_scale, BAIL_TYPE_RANGE,
                         G_IMPLEMENT_INTERFACE (BATK_TYPE_TEXT, batk_text_interface_init))

static void	 
bail_scale_class_init (BailScaleClass *klass)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (klass);
  BatkObjectClass *class = BATK_OBJECT_CLASS (klass);

  class->initialize = bail_scale_real_initialize;

  bobject_class->finalize = bail_scale_finalize;
  bobject_class->notify = bail_scale_notify;
}

static void
bail_scale_init (BailScale      *scale)
{
}

static void
bail_scale_real_initialize (BatkObject *obj,
                            gpointer  data)
{
  BailScale *bail_scale;
  const gchar *txt;
  BangoLayout *layout;

  BATK_OBJECT_CLASS (bail_scale_parent_class)->initialize (obj, data);

  bail_scale = BAIL_SCALE (obj);
  bail_scale->textutil = bail_text_util_new ();

  layout = btk_scale_get_layout (BTK_SCALE (data));
  if (layout)
    {
      txt = bango_layout_get_text (layout);
      if (txt)
        {
          bail_text_util_text_setup (bail_scale->textutil, txt);
        }
    }
}

static void
bail_scale_finalize (BObject *object)
{
  BailScale *scale = BAIL_SCALE (object);

  g_object_unref (scale->textutil);
  B_OBJECT_CLASS (bail_scale_parent_class)->finalize (object);

}

static void
bail_scale_notify (BObject    *obj,
                   BParamSpec *pspec)
{
  BailScale *scale = BAIL_SCALE (obj);

  if (strcmp (pspec->name, "accessible-value") == 0)
    {
      BtkWidget *widget;

      widget = BTK_ACCESSIBLE (obj)->widget;
      if (widget)
        {
          BtkScale *btk_scale;
          BangoLayout *layout;
          const gchar *txt;

          btk_scale = BTK_SCALE (widget);
          layout = btk_scale_get_layout (btk_scale);
          if (layout)
            {
              txt = bango_layout_get_text (layout);
              if (txt)
                {
	          g_signal_emit_by_name (obj, "text_changed::delete", 0,
                                         btk_text_buffer_get_char_count (scale->textutil->buffer));
                  bail_text_util_text_setup (scale->textutil, txt);
	          g_signal_emit_by_name (obj, "text_changed::insert", 0,
                                         g_utf8_strlen (txt, -1));
                }
            }
        }
    }
  B_OBJECT_CLASS (bail_scale_parent_class)->notify (obj, pspec);
}

/* batktext.h */

static void
batk_text_interface_init (BatkTextIface *iface)
{
  iface->get_text = bail_scale_get_text;
  iface->get_character_at_offset = bail_scale_get_character_at_offset;
  iface->get_text_before_offset = bail_scale_get_text_before_offset;
  iface->get_text_at_offset = bail_scale_get_text_at_offset;
  iface->get_text_after_offset = bail_scale_get_text_after_offset;
  iface->get_character_count = bail_scale_get_character_count;
  iface->get_character_extents = bail_scale_get_character_extents;
  iface->get_offset_at_point = bail_scale_get_offset_at_point;
  iface->get_run_attributes = bail_scale_get_run_attributes;
  iface->get_default_attributes = bail_scale_get_default_attributes;
}

static gchar*
bail_scale_get_text (BatkText *text,
                     gint    start_pos,
                     gint    end_pos)
{
  BtkWidget *widget;
  BailScale *scale;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  scale = BAIL_SCALE (text);
  return bail_text_util_get_substring (scale->textutil, 
                                       start_pos, end_pos);
}

static gchar*
bail_scale_get_text_before_offset (BatkText         *text,
     				   gint            offset,
				   BatkTextBoundary boundary_type,
				   gint            *start_offset,
				   gint            *end_offset)
{
  BtkWidget *widget;
  BailScale *scale;
  BangoLayout *layout;
  gchar *txt;
  
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  scale = BAIL_SCALE (text);
  layout = btk_scale_get_layout (BTK_SCALE (widget));
  if (layout)
    {
      txt =  bail_text_util_get_text (scale->textutil,
                           layout, BAIL_BEFORE_OFFSET, 
                           boundary_type, offset, start_offset, end_offset); 
    }
  else
    txt = NULL;

  return txt;
}

static gchar*
bail_scale_get_text_at_offset (BatkText         *text,
	   		       gint            offset,
			       BatkTextBoundary boundary_type,
 			       gint            *start_offset,
			       gint            *end_offset)
{
  BtkWidget *widget;
  BailScale *scale;
  BangoLayout *layout;
  gchar *txt;
 
  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  scale = BAIL_SCALE (text);
  layout = btk_scale_get_layout (BTK_SCALE (widget));
  if (layout)
    {
      txt =  bail_text_util_get_text (scale->textutil,
                              layout, BAIL_AT_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
    }
  else
    txt = NULL;

  return txt;
}

static gchar*
bail_scale_get_text_after_offset (BatkText         *text,
				  gint            offset,
				  BatkTextBoundary boundary_type,
				  gint            *start_offset,
				  gint            *end_offset)
{
  BtkWidget *widget;
  BailScale *scale;
  BangoLayout *layout;
  gchar *txt;

  widget = BTK_ACCESSIBLE (text)->widget;
  
  if (widget == NULL)
    /* State is defunct */
    return NULL;
  
  scale = BAIL_SCALE (text);
  layout = btk_scale_get_layout (BTK_SCALE (widget));
  if (layout)
    {
      txt =  bail_text_util_get_text (scale->textutil,
                              layout, BAIL_AFTER_OFFSET, 
                              boundary_type, offset, start_offset, end_offset);
    }
  else
    txt = NULL;

  return txt;
}

static gint
bail_scale_get_character_count (BatkText *text)
{
  BtkWidget *widget;
  BailScale *scale;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return 0;

  scale = BAIL_SCALE (text);
  if (scale->textutil->buffer)
    return btk_text_buffer_get_char_count (scale->textutil->buffer);
  else
    return 0;

}

static void
bail_scale_get_character_extents (BatkText      *text,
				  gint         offset,
		                  gint         *x,
                    		  gint 	       *y,
                                  gint 	       *width,
                                  gint 	       *height,
			          BatkCoordType coords)
{
  BtkWidget *widget;
  BtkScale *scale;
  BangoRectangle char_rect;
  BangoLayout *layout;
  gint index, x_layout, y_layout;
  const gchar *scale_text;
 
  widget = BTK_ACCESSIBLE (text)->widget;

  if (widget == NULL)
    /* State is defunct */
    return;

  scale = BTK_SCALE (widget);
  layout = btk_scale_get_layout (scale);
  if (!layout)
    return;
  scale_text = bango_layout_get_text (layout);
  if (!scale_text)
    return;
  index = g_utf8_offset_to_pointer (scale_text, offset) - scale_text;
  btk_scale_get_layout_offsets (scale, &x_layout, &y_layout);
  bango_layout_index_to_pos (layout, index, &char_rect);
  bail_misc_get_extents_from_bango_rectangle (widget, &char_rect, 
                    x_layout, y_layout, x, y, width, height, coords);
} 

static gint 
bail_scale_get_offset_at_point (BatkText      *text,
                                gint         x,
                                gint         y,
	          		BatkCoordType coords)
{ 
  BtkWidget *widget;
  BtkScale *scale;
  BangoLayout *layout;
  gint index, x_layout, y_layout;
  const gchar *scale_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return -1;

  scale = BTK_SCALE (widget);
  layout = btk_scale_get_layout (scale);
  if (!layout)
    return -1;
  scale_text = bango_layout_get_text (layout);
  if (!scale_text)
    return -1;
  
  btk_scale_get_layout_offsets (scale, &x_layout, &y_layout);
  index = bail_misc_get_index_at_point_in_layout (widget, 
                                              layout, 
                                              x_layout, y_layout, x, y, coords);
  if (index == -1)
    {
      if (coords == BATK_XY_WINDOW || coords == BATK_XY_SCREEN)
        index = g_utf8_strlen (scale_text, -1);
    }
  else
    index = g_utf8_pointer_to_offset (scale_text, scale_text + index);  

  return index;
}

static BatkAttributeSet*
bail_scale_get_run_attributes (BatkText *text,
                               gint    offset,
                               gint    *start_offset,
	                       gint    *end_offset)
{
  BtkWidget *widget;
  BtkScale *scale;
  BatkAttributeSet *at_set = NULL;
  BtkTextDirection dir;
  BangoLayout *layout;
  const gchar *scale_text;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  scale = BTK_SCALE (widget);

  layout = btk_scale_get_layout (scale);
  if (!layout)
    return at_set;
  scale_text = bango_layout_get_text (layout);
  if (!scale_text)
    return at_set;

  dir = btk_widget_get_direction (widget);
  if (dir == BTK_TEXT_DIR_RTL)
    {
      at_set = bail_misc_add_attribute (at_set, 
                                        BATK_TEXT_ATTR_DIRECTION,
     g_strdup (batk_text_attribute_get_value (BATK_TEXT_ATTR_DIRECTION, dir)));
    }

  at_set = bail_misc_layout_get_run_attributes (at_set,
                                                layout,
                                                (gchar *)scale_text,
                                                offset,
                                                start_offset,
                                                end_offset);
  return at_set;
}

static BatkAttributeSet*
bail_scale_get_default_attributes (BatkText *text)
{
  BtkWidget *widget;
  BatkAttributeSet *at_set = NULL;
  BangoLayout *layout;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  layout = btk_scale_get_layout (BTK_SCALE (widget));
  if (layout)
    {
      at_set = bail_misc_get_default_attributes (at_set,
                                                 layout,
                                                 widget);
    }
  return at_set;
}

static gunichar 
bail_scale_get_character_at_offset (BatkText *text,
                                    gint    offset)
{
  BtkWidget *widget;
  BtkScale *scale;
  BangoLayout *layout;
  const gchar *string;
  gchar *index;
  gunichar c;

  widget = BTK_ACCESSIBLE (text)->widget;
  if (widget == NULL)
    /* State is defunct */
    return '\0';

  scale = BTK_SCALE (widget);

  layout = btk_scale_get_layout (scale);
  if (!layout)
    return '\0';
  string = bango_layout_get_text (layout);
  if (offset >= g_utf8_strlen (string, -1))
    c = '\0';
  else
    {
      index = g_utf8_offset_to_pointer (string, offset);

      c = g_utf8_get_char (index);
    }

  return c;
}
