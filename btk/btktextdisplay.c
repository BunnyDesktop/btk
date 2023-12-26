/* btktextdisplay.c - display layed-out text
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000 Red Hat, Inc.
 * Tk->Btk port by Havoc Pennington
 *
 * This file can be used under your choice of two licenses, the LGPL
 * and the original Tk license.
 *
 * LGPL:
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Original Tk license:
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */
/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#define BTK_TEXT_USE_INTERNAL_UNSUPPORTED_API
#undef BDK_DISABLE_DEPRECATED

#include "config.h"
#include "btktextdisplay.h"
#include "btkintl.h"
#include "btkalias.h"
/* DO NOT go putting private headers in here. This file should only
 * use the semi-public headers, as with btktextview.c.
 */

#define BTK_TYPE_TEXT_RENDERER            (_btk_text_renderer_get_type())
#define BTK_TEXT_RENDERER(object)         (B_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_TEXT_RENDERER, BtkTextRenderer))
#define BTK_IS_TEXT_RENDERER(object)      (B_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_TEXT_RENDERER))
#define BTK_TEXT_RENDERER_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_RENDERER, BtkTextRendererClass))
#define BTK_IS_TEXT_RENDERER_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_RENDERER))
#define BTK_TEXT_RENDERER_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_RENDERER, BtkTextRendererClass))

typedef struct _BtkTextRenderer      BtkTextRenderer;
typedef struct _BtkTextRendererClass BtkTextRendererClass;

enum {
  NORMAL,
  SELECTED,
  CURSOR
};

struct _BtkTextRenderer
{
  BdkBangoRenderer parent_instance;

  BdkScreen *screen;

  BtkWidget *widget;
  BdkDrawable *drawable;
  BdkRectangle clip_rect;
  
  BdkColor *error_color;	/* Error underline color for this widget */
  GList *widgets;		/* widgets encountered when drawing */
  
  int state;
};

struct _BtkTextRendererClass
{
  BdkBangoRendererClass parent_class;
};

G_DEFINE_TYPE (BtkTextRenderer, _btk_text_renderer, BDK_TYPE_BANGO_RENDERER)

static BdkColor *
text_renderer_get_error_color (BtkTextRenderer *text_renderer)
{
  static const BdkColor red = { 0, 0xffff, 0, 0 };

  if (!text_renderer->error_color)
    btk_widget_style_get (text_renderer->widget,
			  "error-underline-color", &text_renderer->error_color,
			  NULL);
  
  if (!text_renderer->error_color)
    text_renderer->error_color = bdk_color_copy (&red);

  return text_renderer->error_color;
}

static void
text_renderer_set_bdk_color (BtkTextRenderer *text_renderer,
			     BangoRenderPart  part,
			     BdkColor        *bdk_color)
{
  BangoRenderer *renderer = BANGO_RENDERER (text_renderer);

  if (bdk_color)
    {
      BangoColor color;

      color.red = bdk_color->red;
      color.green = bdk_color->green;
      color.blue = bdk_color->blue;
      
      bango_renderer_set_color (renderer, part, &color);
    }
  else
    bango_renderer_set_color (renderer, part, NULL);
	   
}

static BtkTextAppearance *
get_item_appearance (BangoItem *item)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  while (tmp_list)
    {
      BangoAttribute *attr = tmp_list->data;

      if (attr->klass->type == btk_text_attr_appearance_type)
	return &((BtkTextAttrAppearance *)attr)->appearance;

      tmp_list = tmp_list->next;
    }

  return NULL;
}

static void
btk_text_renderer_prepare_run (BangoRenderer  *renderer,
			       BangoLayoutRun *run)
{
  BtkTextRenderer *text_renderer = BTK_TEXT_RENDERER (renderer);
  BdkBangoRenderer *bdk_renderer = BDK_BANGO_RENDERER (renderer);
  BdkColor *bg_color, *fg_color, *underline_color;
  BdkPixmap *fg_stipple, *bg_stipple;
  BtkTextAppearance *appearance;

  BANGO_RENDERER_CLASS (_btk_text_renderer_parent_class)->prepare_run (renderer, run);

  appearance = get_item_appearance (run->item);
  g_assert (appearance != NULL);

  if (appearance->draw_bg && text_renderer->state == NORMAL)
    bg_color = &appearance->bg_color;
  else
    bg_color = NULL;
  
  text_renderer_set_bdk_color (text_renderer, BANGO_RENDER_PART_BACKGROUND, bg_color);

  if (text_renderer->state == SELECTED)
    {
      if (btk_widget_has_focus (text_renderer->widget))
	fg_color = &text_renderer->widget->style->text[BTK_STATE_SELECTED];
      else
	fg_color = &text_renderer->widget->style->text[BTK_STATE_ACTIVE];
    }
  else if (text_renderer->state == CURSOR && btk_widget_has_focus (text_renderer->widget))
    fg_color = &text_renderer->widget->style->base[BTK_STATE_NORMAL];
  else
    fg_color = &appearance->fg_color;

  text_renderer_set_bdk_color (text_renderer, BANGO_RENDER_PART_FOREGROUND, fg_color);
  text_renderer_set_bdk_color (text_renderer, BANGO_RENDER_PART_STRIKETHROUGH, fg_color);

  if (appearance->underline == BANGO_UNDERLINE_ERROR)
    underline_color = text_renderer_get_error_color (text_renderer);
  else
    underline_color = fg_color;

  text_renderer_set_bdk_color (text_renderer, BANGO_RENDER_PART_UNDERLINE, underline_color);

  fg_stipple = appearance->fg_stipple;
  if (fg_stipple && text_renderer->screen != bdk_drawable_get_screen (fg_stipple))
    {
      g_warning ("btk_text_renderer_prepare_run:\n"
		 "The foreground stipple bitmap has been created on the wrong screen.\n"
		 "Ignoring the stipple bitmap information.");
      fg_stipple = NULL;
    }
      
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_FOREGROUND, fg_stipple);
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_STRIKETHROUGH, fg_stipple);
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_UNDERLINE, fg_stipple);

  bg_stipple = appearance->draw_bg ? appearance->bg_stipple : NULL;
  
  if (bg_stipple && text_renderer->screen != bdk_drawable_get_screen (bg_stipple))
    {
      g_warning ("btk_text_renderer_prepare_run:\n"
		 "The background stipple bitmap has been created on the wrong screen.\n"
		 "Ignoring the stipple bitmap information.");
      bg_stipple = NULL;
    }
  
  bdk_bango_renderer_set_stipple (bdk_renderer, BANGO_RENDER_PART_BACKGROUND, bg_stipple);
}

static void
btk_text_renderer_draw_shape (BangoRenderer   *renderer,
			      BangoAttrShape  *attr,
			      int              x,
			      int              y)
{
  BtkTextRenderer *text_renderer = BTK_TEXT_RENDERER (renderer);
  BdkGC *fg_gc;

  if (text_renderer->state == SELECTED)
    {
      if (btk_widget_has_focus (text_renderer->widget))
	fg_gc = text_renderer->widget->style->text_gc[BTK_STATE_SELECTED];
      else
	fg_gc = text_renderer->widget->style->text_gc[BTK_STATE_SELECTED];
    }
  else if (text_renderer->state == CURSOR && btk_widget_has_focus (text_renderer->widget))
    fg_gc = text_renderer->widget->style->base_gc[BTK_STATE_NORMAL];
  else
    fg_gc = text_renderer->widget->style->text_gc[BTK_STATE_NORMAL];
  
  if (attr->data == NULL)
    {
      /* This happens if we have an empty widget anchor. Draw
       * something empty-looking.
       */
      BdkRectangle shape_rect, draw_rect;
      
      shape_rect.x = BANGO_PIXELS (x);
      shape_rect.y = BANGO_PIXELS (y + attr->logical_rect.y);
      shape_rect.width = BANGO_PIXELS (x + attr->logical_rect.width) - shape_rect.x;
      shape_rect.height = BANGO_PIXELS (y + attr->logical_rect.y + attr->logical_rect.height) - shape_rect.y;
      
      if (bdk_rectangle_intersect (&shape_rect, &text_renderer->clip_rect,
				   &draw_rect))
	{
	  bdk_draw_rectangle (text_renderer->drawable, fg_gc,
			      FALSE, shape_rect.x, shape_rect.y,
			      shape_rect.width, shape_rect.height);
	  
	  bdk_draw_line (text_renderer->drawable, fg_gc,
			 shape_rect.x, shape_rect.y,
			 shape_rect.x + shape_rect.width,
			 shape_rect.y + shape_rect.height);
	  
	  bdk_draw_line (text_renderer->drawable, fg_gc,
			 shape_rect.x + shape_rect.width, shape_rect.y,
			 shape_rect.x,
			 shape_rect.y + shape_rect.height);
	}
    }
  else if (BDK_IS_PIXBUF (attr->data))
    {
      gint width, height;
      BdkRectangle pixbuf_rect, draw_rect;
      BdkPixbuf *pixbuf;
      
      pixbuf = BDK_PIXBUF (attr->data);
      
      width = bdk_pixbuf_get_width (pixbuf);
      height = bdk_pixbuf_get_height (pixbuf);
      
      pixbuf_rect.x = BANGO_PIXELS (x);
      pixbuf_rect.y = BANGO_PIXELS (y) - height;
      pixbuf_rect.width = width;
      pixbuf_rect.height = height;
      
      if (bdk_rectangle_intersect (&pixbuf_rect, &text_renderer->clip_rect,
				   &draw_rect))
	{
	  bdk_draw_pixbuf (text_renderer->drawable,
			   fg_gc,
			   pixbuf,
			   draw_rect.x - pixbuf_rect.x,
			   draw_rect.y - pixbuf_rect.y,
			   draw_rect.x, draw_rect.y,
			   draw_rect.width,
			   draw_rect.height,
			   BDK_RGB_DITHER_NORMAL,
			   0, 0);
	}
    }
  else if (BTK_IS_WIDGET (attr->data))
    {
      BtkWidget *widget;
      
      widget = BTK_WIDGET (attr->data);

      text_renderer->widgets = g_list_prepend (text_renderer->widgets,
					       g_object_ref (widget));
    }
  else
    g_assert_not_reached (); /* not a pixbuf or widget */
}

static void
btk_text_renderer_finalize (BObject *object)
{
  B_OBJECT_CLASS (_btk_text_renderer_parent_class)->finalize (object);
}

static void
_btk_text_renderer_init (BtkTextRenderer *renderer)
{
}

static void
_btk_text_renderer_class_init (BtkTextRendererClass *klass)
{
  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  
  BangoRendererClass *renderer_class = BANGO_RENDERER_CLASS (klass);
  
  renderer_class->prepare_run = btk_text_renderer_prepare_run;
  renderer_class->draw_shape = btk_text_renderer_draw_shape;

  object_class->finalize = btk_text_renderer_finalize;
}

static void
text_renderer_set_state (BtkTextRenderer *text_renderer,
			 int              state)
{
  text_renderer->state = state;
}

static void
text_renderer_begin (BtkTextRenderer *text_renderer,
		     BtkWidget       *widget,
		     BdkDrawable     *drawable,
		     BdkRectangle    *clip_rect)
{
  text_renderer->widget = widget;
  text_renderer->drawable = drawable;
  text_renderer->clip_rect = *clip_rect;

  bdk_bango_renderer_set_drawable (BDK_BANGO_RENDERER (text_renderer), drawable);
  bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer),
			     widget->style->text_gc[widget->state]);
}

/* Returns a GSList of (referenced) widgets encountered while drawing.
 */
static GList *
text_renderer_end (BtkTextRenderer *text_renderer)
{
  GList *widgets = text_renderer->widgets;

  text_renderer->widget = NULL;
  text_renderer->drawable = NULL;

  text_renderer->widgets = NULL;

  if (text_renderer->error_color)
    {
      bdk_color_free (text_renderer->error_color);
      text_renderer->error_color = NULL;
    }

  bdk_bango_renderer_set_drawable (BDK_BANGO_RENDERER (text_renderer), NULL);
  bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), NULL);

  return widgets;
}


static BdkRebunnyion *
get_selected_clip (BtkTextRenderer    *text_renderer,
                   BangoLayout        *layout,
                   BangoLayoutLine    *line,
                   int                 x,
                   int                 y,
                   int                 height,
                   int                 start_index,
                   int                 end_index)
{
  gint *ranges;
  gint n_ranges, i;
  BdkRebunnyion *clip_rebunnyion = bdk_rebunnyion_new ();
  BdkRebunnyion *tmp_rebunnyion;

  bango_layout_line_get_x_ranges (line, start_index, end_index, &ranges, &n_ranges);

  for (i=0; i < n_ranges; i++)
    {
      BdkRectangle rect;

      rect.x = x + BANGO_PIXELS (ranges[2*i]);
      rect.y = y;
      rect.width = BANGO_PIXELS (ranges[2*i + 1]) - BANGO_PIXELS (ranges[2*i]);
      rect.height = height;
      
      bdk_rebunnyion_union_with_rect (clip_rebunnyion, &rect);
    }

  tmp_rebunnyion = bdk_rebunnyion_rectangle (&text_renderer->clip_rect);
  bdk_rebunnyion_intersect (clip_rebunnyion, tmp_rebunnyion);
  bdk_rebunnyion_destroy (tmp_rebunnyion);

  g_free (ranges);
  return clip_rebunnyion;
}

static void
render_para (BtkTextRenderer    *text_renderer,
             BtkTextLineDisplay *line_display,
             /* Top-left corner of paragraph including all margins */
             int                 x,
             int                 y,
             int                 selection_start_index,
             int                 selection_end_index)
{
  BangoLayout *layout = line_display->layout;
  int byte_offset = 0;
  BangoLayoutIter *iter;
  BangoRectangle layout_logical;
  int screen_width;
  BdkGC *selection_gc, *fg_gc;
  gint state;
  
  gboolean first = TRUE;

  iter = bango_layout_get_iter (layout);

  bango_layout_iter_get_layout_extents (iter, NULL, &layout_logical);

  /* Adjust for margins */
  
  layout_logical.x += line_display->x_offset * BANGO_SCALE;
  layout_logical.y += line_display->top_margin * BANGO_SCALE;

  screen_width = line_display->total_width;
  
  if (btk_widget_has_focus (text_renderer->widget))
    state = BTK_STATE_SELECTED;
  else
    state = BTK_STATE_ACTIVE;

  selection_gc = text_renderer->widget->style->base_gc [state];
  fg_gc = text_renderer->widget->style->text_gc[text_renderer->widget->state];

  do
    {
      BangoLayoutLine *line = bango_layout_iter_get_line_readonly (iter);
      int selection_y, selection_height;
      int first_y, last_y;
      BangoRectangle line_rect;
      int baseline;
      gboolean at_last_line;
      
      bango_layout_iter_get_line_extents (iter, NULL, &line_rect);
      baseline = bango_layout_iter_get_baseline (iter);
      bango_layout_iter_get_line_yrange (iter, &first_y, &last_y);
      
      /* Adjust for margins */

      line_rect.x += line_display->x_offset * BANGO_SCALE;
      line_rect.y += line_display->top_margin * BANGO_SCALE;
      baseline += line_display->top_margin * BANGO_SCALE;

      /* Selection is the height of the line, plus top/bottom
       * margin if we're the first/last line
       */
      selection_y = y + BANGO_PIXELS (first_y) + line_display->top_margin;
      selection_height = BANGO_PIXELS (last_y) - BANGO_PIXELS (first_y);

      if (first)
        {
          selection_y -= line_display->top_margin;
          selection_height += line_display->top_margin;
        }

      at_last_line = bango_layout_iter_at_last_line (iter);
      if (at_last_line)
        selection_height += line_display->bottom_margin;
      
      first = FALSE;

      if (selection_start_index < byte_offset &&
          selection_end_index > line->length + byte_offset) /* All selected */
        {
          bdk_draw_rectangle (text_renderer->drawable,
                              selection_gc,
                              TRUE,
                              x + line_display->left_margin,
                              selection_y,
                              screen_width,
                              selection_height);

	  text_renderer_set_state (text_renderer, SELECTED);
	  bango_renderer_draw_layout_line (BANGO_RENDERER (text_renderer),
					   line, 
					   BANGO_SCALE * x + line_rect.x,
					   BANGO_SCALE * y + baseline);
        }
      else
        {
          if (line_display->pg_bg_color)
            {
              BdkGC *bg_gc;
              
              bg_gc = bdk_gc_new (text_renderer->drawable);
              bdk_gc_set_fill (bg_gc, BDK_SOLID);
              bdk_gc_set_rgb_fg_color (bg_gc, line_display->pg_bg_color);
            
              bdk_draw_rectangle (text_renderer->drawable,
                                  bg_gc,
                                  TRUE,
                                  x + line_display->left_margin,
                                  selection_y,
                                  screen_width,
                                  selection_height);
              
              g_object_unref (bg_gc);
            }
        
	  text_renderer_set_state (text_renderer, NORMAL);
	  bango_renderer_draw_layout_line (BANGO_RENDERER (text_renderer),
					   line, 
					   BANGO_SCALE * x + line_rect.x,
					   BANGO_SCALE * y + baseline);

	  /* Check if some part of the line is selected; the newline
	   * that is after line->length for the last line of the
	   * paragraph counts as part of the line for this
	   */
          if ((selection_start_index < byte_offset + line->length ||
	       (selection_start_index == byte_offset + line->length && bango_layout_iter_at_last_line (iter))) &&
	      selection_end_index > byte_offset)
            {
              BdkRebunnyion *clip_rebunnyion = get_selected_clip (text_renderer, layout, line,
                                                          x + line_display->x_offset,
                                                          selection_y,
                                                          selection_height,
                                                          selection_start_index, selection_end_index);

	      /* When we change the clip on the foreground GC, we have to set
	       * it on the rendererer again, since the rendererer might have
	       * copied the GC to change attributes.
	       */
	      bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), NULL);
              bdk_gc_set_clip_rebunnyion (selection_gc, clip_rebunnyion);
              bdk_gc_set_clip_rebunnyion (fg_gc, clip_rebunnyion);
	      bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), fg_gc);

              bdk_draw_rectangle (text_renderer->drawable,
                                  selection_gc,
                                  TRUE,
                                  x + BANGO_PIXELS (line_rect.x),
                                  selection_y,
                                  BANGO_PIXELS (line_rect.width),
                                  selection_height);

	      text_renderer_set_state (text_renderer, SELECTED);
	      bango_renderer_draw_layout_line (BANGO_RENDERER (text_renderer),
					       line, 
					       BANGO_SCALE * x + line_rect.x,
					       BANGO_SCALE * y + baseline);

	      bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), NULL);
              bdk_gc_set_clip_rebunnyion (selection_gc, NULL);
              bdk_gc_set_clip_rebunnyion (fg_gc, NULL);
	      bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), fg_gc);
	      
              bdk_rebunnyion_destroy (clip_rebunnyion);

              /* Paint in the ends of the line */
              if (line_rect.x > line_display->left_margin * BANGO_SCALE &&
                  ((line_display->direction == BTK_TEXT_DIR_LTR && selection_start_index < byte_offset) ||
                   (line_display->direction == BTK_TEXT_DIR_RTL && selection_end_index > byte_offset + line->length)))
                {
                  bdk_draw_rectangle (text_renderer->drawable,
                                      selection_gc,
                                      TRUE,
                                      x + line_display->left_margin,
                                      selection_y,
                                      BANGO_PIXELS (line_rect.x) - line_display->left_margin,
                                      selection_height);
                }

              if (line_rect.x + line_rect.width <
                  (screen_width + line_display->left_margin) * BANGO_SCALE &&
                  ((line_display->direction == BTK_TEXT_DIR_LTR && selection_end_index > byte_offset + line->length) ||
                   (line_display->direction == BTK_TEXT_DIR_RTL && selection_start_index < byte_offset)))
                {
                  int nonlayout_width;

                  nonlayout_width =
                    line_display->left_margin + screen_width -
                    BANGO_PIXELS (line_rect.x) - BANGO_PIXELS (line_rect.width);

                  bdk_draw_rectangle (text_renderer->drawable,
                                      selection_gc,
                                      TRUE,
                                      x + BANGO_PIXELS (line_rect.x) + BANGO_PIXELS (line_rect.width),
                                      selection_y,
                                      nonlayout_width,
                                      selection_height);
                }
            }
	  else if (line_display->has_block_cursor &&
		   btk_widget_has_focus (text_renderer->widget) &&
		   byte_offset <= line_display->insert_index &&
		   (line_display->insert_index < byte_offset + line->length ||
		    (at_last_line && line_display->insert_index == byte_offset + line->length)))
	    {
	      BdkRectangle cursor_rect;
	      BdkGC *cursor_gc;

	      /* we draw text using base color on filled cursor rectangle of cursor color
	       * (normally white on black) */
	      cursor_gc = _btk_widget_get_cursor_gc (text_renderer->widget);

	      cursor_rect.x = x + line_display->x_offset + line_display->block_cursor.x;
	      cursor_rect.y = y + line_display->block_cursor.y + line_display->top_margin;
	      cursor_rect.width = line_display->block_cursor.width;
	      cursor_rect.height = line_display->block_cursor.height;

	      bdk_gc_set_clip_rectangle (cursor_gc, &cursor_rect);

              bdk_draw_rectangle (text_renderer->drawable,
                                  cursor_gc,
                                  TRUE,
                                  cursor_rect.x,
                                  cursor_rect.y,
                                  cursor_rect.width,
                                  cursor_rect.height);

              bdk_gc_set_clip_rebunnyion (cursor_gc, NULL);

	      /* draw text under the cursor if any */
	      if (!line_display->cursor_at_line_end)
		{
		  BdkGC *cursor_text_gc;

		  cursor_text_gc = text_renderer->widget->style->base_gc[text_renderer->widget->state];
		  bdk_gc_set_clip_rectangle (cursor_text_gc, &cursor_rect);

		  bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), cursor_text_gc);
		  text_renderer_set_state (text_renderer, CURSOR);

		  bango_renderer_draw_layout_line (BANGO_RENDERER (text_renderer),
						   line,
						   BANGO_SCALE * x + line_rect.x,
						   BANGO_SCALE * y + baseline);

		  bdk_bango_renderer_set_gc (BDK_BANGO_RENDERER (text_renderer), fg_gc);
		  bdk_gc_set_clip_rebunnyion (cursor_text_gc, NULL);
		}
	    }
        }

      byte_offset += line->length;
    }
  while (bango_layout_iter_next_line (iter));

  bango_layout_iter_free (iter);
}

static void
on_renderer_display_closed (BdkDisplay       *display,
                            gboolean          is_error,
			    BtkTextRenderer  *text_renderer)
{
  g_signal_handlers_disconnect_by_func (text_renderer->screen,
					(gpointer)on_renderer_display_closed,
					text_renderer);
  g_object_set_data (B_OBJECT (text_renderer->screen), I_("btk-text-renderer"), NULL);
}

static BtkTextRenderer *
get_text_renderer (BdkScreen *screen)
{
  BtkTextRenderer *text_renderer;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);
  
  text_renderer = g_object_get_data (B_OBJECT (screen), "btk-text-renderer");
  if (!text_renderer)
    {
      text_renderer = g_object_new (BTK_TYPE_TEXT_RENDERER, "screen", screen, NULL);
      text_renderer->screen = screen;
      
      g_object_set_data_full (B_OBJECT (screen), I_("btk-text-renderer"), text_renderer,
			      (GDestroyNotify)g_object_unref);

      g_signal_connect_object (bdk_screen_get_display (screen), "closed",
                               G_CALLBACK (on_renderer_display_closed),
                               text_renderer, 0);
    }

  return text_renderer;
}

void
btk_text_layout_draw (BtkTextLayout *layout,
                      BtkWidget *widget,
                      BdkDrawable *drawable,
		      BdkGC       *cursor_gc,
                      /* Location of the drawable
                         in layout coordinates */
                      gint x_offset,
                      gint y_offset,
                      /* Rebunnyion of the layout to
                         render */
                      gint x,
                      gint y,
                      gint width,
                      gint height,
                      /* widgets to expose */
                      GList **widgets)
{
  BdkRectangle clip;
  gint current_y;
  GSList *cursor_list;
  BtkTextRenderer *text_renderer;
  BtkTextIter selection_start, selection_end;
  gboolean have_selection = FALSE;
  GSList *line_list;
  GSList *tmp_list;
  GList *tmp_widgets;
  
  g_return_if_fail (BTK_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (layout->default_style != NULL);
  g_return_if_fail (layout->buffer != NULL);
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  if (width == 0 || height == 0)
    return;

  line_list =  btk_text_layout_get_lines (layout, y + y_offset, y + y_offset + height, &current_y);
  current_y -= y_offset;

  if (line_list == NULL)
    return; /* nothing on the screen */

  clip.x = x;
  clip.y = y;
  clip.width = width;
  clip.height = height;

  text_renderer = get_text_renderer (bdk_drawable_get_screen (drawable));

  text_renderer_begin (text_renderer, widget, drawable, &clip);

  btk_text_layout_wrap_loop_start (layout);

  if (btk_text_buffer_get_selection_bounds (layout->buffer,
                                            &selection_start,
                                            &selection_end))
    have_selection = TRUE;

  tmp_list = line_list;
  while (tmp_list != NULL)
    {
      BtkTextLineDisplay *line_display;
      gint selection_start_index = -1;
      gint selection_end_index = -1;
      gboolean have_strong;
      gboolean have_weak;

      BtkTextLine *line = tmp_list->data;

      line_display = btk_text_layout_get_line_display (layout, line, FALSE);

      if (line_display->height > 0)
        {
          g_assert (line_display->layout != NULL);
          
          if (have_selection)
            {
              BtkTextIter line_start, line_end;
              gint byte_count;
              
              btk_text_layout_get_iter_at_line (layout,
                                                &line_start,
                                                line, 0);
              line_end = line_start;
	      if (!btk_text_iter_ends_line (&line_end))
		btk_text_iter_forward_to_line_end (&line_end);
              byte_count = btk_text_iter_get_visible_line_index (&line_end);     

              if (btk_text_iter_compare (&selection_start, &line_end) <= 0 &&
                  btk_text_iter_compare (&selection_end, &line_start) >= 0)
                {
                  if (btk_text_iter_compare (&selection_start, &line_start) >= 0)
                    selection_start_index = btk_text_iter_get_visible_line_index (&selection_start);
                  else
                    selection_start_index = -1;

                  if (btk_text_iter_compare (&selection_end, &line_end) <= 0)
                    selection_end_index = btk_text_iter_get_visible_line_index (&selection_end);
                  else
                    selection_end_index = byte_count + 1; /* + 1 to flag past-the-end */
                }
            }

          render_para (text_renderer, line_display,
                       - x_offset,
                       current_y,
                       selection_start_index, selection_end_index);

          /* We paint the cursors last, because they overlap another chunk
         and need to appear on top. */

 	  have_strong = FALSE;
 	  have_weak = FALSE;
	  
	  cursor_list = line_display->cursors;
	  while (cursor_list)
	    {
	      BtkTextCursorDisplay *cursor = cursor_list->data;
 	      if (cursor->is_strong)
 		have_strong = TRUE;
 	      else
 		have_weak = TRUE;
	      
	      cursor_list = cursor_list->next;
 	    }
	  
          cursor_list = line_display->cursors;
          while (cursor_list)
            {
              BtkTextCursorDisplay *cursor = cursor_list->data;
	      BtkTextDirection dir;
 	      BdkRectangle cursor_location;

              dir = line_display->direction;
 	      if (have_strong && have_weak)
 		{
 		  if (!cursor->is_strong)
 		    dir = (dir == BTK_TEXT_DIR_RTL) ? BTK_TEXT_DIR_LTR : BTK_TEXT_DIR_RTL;
 		}
 
 	      cursor_location.x = line_display->x_offset + cursor->x - x_offset;
 	      cursor_location.y = current_y + line_display->top_margin + cursor->y;
 	      cursor_location.width = 0;
 	      cursor_location.height = cursor->height;

	      btk_draw_insertion_cursor (widget, drawable, &clip, &cursor_location,
					 cursor->is_strong,
					 dir, have_strong && have_weak);

              cursor_list = cursor_list->next;
            }
        } /* line_display->height > 0 */
          
      current_y += line_display->height;
      btk_text_layout_free_line_display (layout, line_display);
      
      tmp_list = b_slist_next (tmp_list);
    }

  btk_text_layout_wrap_loop_end (layout);

  tmp_widgets = text_renderer_end (text_renderer);
  if (widgets)
    *widgets = tmp_widgets;
  else
    {
      g_list_foreach (tmp_widgets, (GFunc)g_object_unref, NULL);
      g_list_free (tmp_widgets);
    }

  b_slist_free (line_list);
}

#define __BTK_TEXT_DISPLAY_C__
#include "btkaliasdef.c"
