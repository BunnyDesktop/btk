/* BTK - The GIMP Toolkit
 * btkprintcontext.c: Print Context
 * Copyright (C) 2006, Red Hat, Inc.
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
#include "btkprintoperation-private.h"
#include "btkalias.h"

typedef struct _BtkPrintContextClass BtkPrintContextClass;

#define BTK_IS_PRINT_CONTEXT_CLASS(klass)  (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PRINT_CONTEXT))
#define BTK_PRINT_CONTEXT_CLASS(klass)     (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PRINT_CONTEXT, BtkPrintContextClass))
#define BTK_PRINT_CONTEXT_GET_CLASS(obj)   (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PRINT_CONTEXT, BtkPrintContextClass))

#define MM_PER_INCH 25.4
#define POINTS_PER_INCH 72

struct _BtkPrintContext
{
  BObject parent_instance;

  BtkPrintOperation *op;
  bairo_t *cr;
  BtkPageSetup *page_setup;

  gdouble surface_dpi_x;
  gdouble surface_dpi_y;
  
  gdouble pixels_per_unit_x;
  gdouble pixels_per_unit_y;

  gboolean has_hard_margins;
  gdouble hard_margin_top;
  gdouble hard_margin_bottom;
  gdouble hard_margin_left;
  gdouble hard_margin_right;

};

struct _BtkPrintContextClass
{
  BObjectClass parent_class;
};

G_DEFINE_TYPE (BtkPrintContext, btk_print_context, B_TYPE_OBJECT)

static void
btk_print_context_finalize (BObject *object)
{
  BtkPrintContext *context = BTK_PRINT_CONTEXT (object);

  if (context->page_setup)
    g_object_unref (context->page_setup);

  if (context->cr)
    bairo_destroy (context->cr);
  
  B_OBJECT_CLASS (btk_print_context_parent_class)->finalize (object);
}

static void
btk_print_context_init (BtkPrintContext *context)
{
}

static void
btk_print_context_class_init (BtkPrintContextClass *class)
{
  BObjectClass *bobject_class = (BObjectClass *)class;

  bobject_class->finalize = btk_print_context_finalize;
}


BtkPrintContext *
_btk_print_context_new (BtkPrintOperation *op)
{
  BtkPrintContext *context;

  context = g_object_new (BTK_TYPE_PRINT_CONTEXT, NULL);

  context->op = op;
  context->cr = NULL;
  context->has_hard_margins = FALSE;
  
  return context;
}

static BangoFontMap *
_btk_print_context_get_fontmap (BtkPrintContext *context)
{
  return bango_bairo_font_map_get_default ();
}

/**
 * btk_print_context_set_bairo_context:
 * @context: a #BtkPrintContext
 * @cr: the bairo context
 * @dpi_x: the horizontal resolution to use with @cr
 * @dpi_y: the vertical resolution to use with @cr
 *
 * Sets a new bairo context on a print context. 
 * 
 * This function is intended to be used when implementing
 * an internal print preview, it is not needed for printing,
 * since BTK+ itself creates a suitable bairo context in that
 * case.
 *
 * Since: 2.10 
 */
void
btk_print_context_set_bairo_context (BtkPrintContext *context,
				     bairo_t         *cr,
				     double           dpi_x,
				     double           dpi_y)
{
  if (context->cr)
    bairo_destroy (context->cr);

  context->cr = bairo_reference (cr);
  context->surface_dpi_x = dpi_x;
  context->surface_dpi_y = dpi_y;

  switch (context->op->priv->unit)
    {
    default:
    case BTK_UNIT_PIXEL:
      /* Do nothing, this is the bairo default unit */
      context->pixels_per_unit_x = 1.0;
      context->pixels_per_unit_y = 1.0;
      break;
    case BTK_UNIT_POINTS:
      context->pixels_per_unit_x = dpi_x / POINTS_PER_INCH;
      context->pixels_per_unit_y = dpi_y / POINTS_PER_INCH;
      break;
    case BTK_UNIT_INCH:
      context->pixels_per_unit_x = dpi_x;
      context->pixels_per_unit_y = dpi_y;
      break;
    case BTK_UNIT_MM:
      context->pixels_per_unit_x = dpi_x / MM_PER_INCH;
      context->pixels_per_unit_y = dpi_y / MM_PER_INCH;
      break;
    }
  bairo_scale (context->cr,
	       context->pixels_per_unit_x,
	       context->pixels_per_unit_y);
}


void
_btk_print_context_rotate_according_to_orientation (BtkPrintContext *context)
{
  bairo_t *cr = context->cr;
  bairo_matrix_t matrix;
  BtkPaperSize *paper_size;
  gdouble width, height;

  paper_size = btk_page_setup_get_paper_size (context->page_setup);

  width = btk_paper_size_get_width (paper_size, BTK_UNIT_INCH);
  width = width * context->surface_dpi_x / context->pixels_per_unit_x;
  height = btk_paper_size_get_height (paper_size, BTK_UNIT_INCH);
  height = height * context->surface_dpi_y / context->pixels_per_unit_y;
  
  switch (btk_page_setup_get_orientation (context->page_setup))
    {
    default:
    case BTK_PAGE_ORIENTATION_PORTRAIT:
      break;
    case BTK_PAGE_ORIENTATION_LANDSCAPE:
      bairo_translate (cr, 0, height);
      bairo_matrix_init (&matrix,
			 0, -1,
			 1,  0,
			 0,  0);
      bairo_transform (cr, &matrix);
      break;
    case BTK_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      bairo_translate (cr, width, height);
      bairo_matrix_init (&matrix,
			 -1,  0,
			  0, -1,
			  0,  0);
      bairo_transform (cr, &matrix);
      break;
    case BTK_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      bairo_translate (cr, width, 0);
      bairo_matrix_init (&matrix,
			  0,  1,
			 -1,  0,
			  0,  0);
      bairo_transform (cr, &matrix);
      break;
    }
}

void
_btk_print_context_translate_into_margin (BtkPrintContext *context)
{
  BtkPrintOperationPrivate *priv;
  gdouble left, top;

  g_return_if_fail (BTK_IS_PRINT_CONTEXT (context));

  priv = context->op->priv;

  /* We do it this way to also handle BTK_UNIT_PIXELS */
  
  left = btk_page_setup_get_left_margin (context->page_setup, BTK_UNIT_INCH);
  top = btk_page_setup_get_top_margin (context->page_setup, BTK_UNIT_INCH);

  bairo_translate (context->cr,
		   left * context->surface_dpi_x / context->pixels_per_unit_x,
		   top * context->surface_dpi_y / context->pixels_per_unit_y);
}

void
_btk_print_context_set_page_setup (BtkPrintContext *context,
				   BtkPageSetup    *page_setup)
{
  g_return_if_fail (BTK_IS_PRINT_CONTEXT (context));
  g_return_if_fail (page_setup == NULL ||
		    BTK_IS_PAGE_SETUP (page_setup));
  
  g_object_ref (page_setup);

  if (context->page_setup != NULL)
    g_object_unref (context->page_setup);

  context->page_setup = page_setup;
}

/**
 * btk_print_context_get_bairo_context:
 * @context: a #BtkPrintContext
 *
 * Obtains the bairo context that is associated with the
 * #BtkPrintContext.
 *
 * Return value: (transfer none): the bairo context of @context
 *
 * Since: 2.10
 */
bairo_t *
btk_print_context_get_bairo_context (BtkPrintContext *context)
{
  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), NULL);

  return context->cr;
}

/**
 * btk_print_context_get_page_setup:
 * @context: a #BtkPrintContext
 *
 * Obtains the #BtkPageSetup that determines the page
 * dimensions of the #BtkPrintContext.
 *
 * Return value: (transfer none): the page setup of @context
 *
 * Since: 2.10
 */
BtkPageSetup *
btk_print_context_get_page_setup (BtkPrintContext *context)
{
  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), NULL);

  return context->page_setup;
}

/**
 * btk_print_context_get_width:
 * @context: a #BtkPrintContext
 *
 * Obtains the width of the #BtkPrintContext, in pixels.
 *
 * Return value: the width of @context
 *
 * Since: 2.10 
 */
gdouble
btk_print_context_get_width (BtkPrintContext *context)
{
  BtkPrintOperationPrivate *priv;
  gdouble width;

  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), 0);

  priv = context->op->priv;

  if (priv->use_full_page)
    width = btk_page_setup_get_paper_width (context->page_setup, BTK_UNIT_INCH);
  else
    width = btk_page_setup_get_page_width (context->page_setup, BTK_UNIT_INCH);

  /* Really dpi_x? What about landscape? what does dpi_x mean in that case? */
  return width * context->surface_dpi_x / context->pixels_per_unit_x;
}

/**
 * btk_print_context_get_height:
 * @context: a #BtkPrintContext
 * 
 * Obtains the height of the #BtkPrintContext, in pixels.
 *
 * Return value: the height of @context
 *
 * Since: 2.10
 */
gdouble
btk_print_context_get_height (BtkPrintContext *context)
{
  BtkPrintOperationPrivate *priv;
  gdouble height;

  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), 0);

  priv = context->op->priv;

  if (priv->use_full_page)
    height = btk_page_setup_get_paper_height (context->page_setup, BTK_UNIT_INCH);
  else
    height = btk_page_setup_get_page_height (context->page_setup, BTK_UNIT_INCH);

  /* Really dpi_y? What about landscape? what does dpi_y mean in that case? */
  return height * context->surface_dpi_y / context->pixels_per_unit_y;
}

/**
 * btk_print_context_get_dpi_x:
 * @context: a #BtkPrintContext
 * 
 * Obtains the horizontal resolution of the #BtkPrintContext,
 * in dots per inch.
 *
 * Return value: the horizontal resolution of @context
 *
 * Since: 2.10
 */
gdouble
btk_print_context_get_dpi_x (BtkPrintContext *context)
{
  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), 0);

  return context->surface_dpi_x;
}

/**
 * btk_print_context_get_dpi_y:
 * @context: a #BtkPrintContext
 * 
 * Obtains the vertical resolution of the #BtkPrintContext,
 * in dots per inch.
 *
 * Return value: the vertical resolution of @context
 *
 * Since: 2.10
 */
gdouble
btk_print_context_get_dpi_y (BtkPrintContext *context)
{
  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), 0);

  return context->surface_dpi_y;
}

/**
 * btk_print_context_get_hard_margins:
 * @context: a #BtkPrintContext
 * @top: (out): top hardware printer margin
 * @bottom: (out): bottom hardware printer margin
 * @left: (out): left hardware printer margin
 * @right: (out): right hardware printer margin
 *
 * Obtains the hardware printer margins of the #BtkPrintContext, in units.
 *
 * Return value: %TRUE if the hard margins were retrieved
 *
 * Since: 2.20
 */
gboolean
btk_print_context_get_hard_margins (BtkPrintContext *context,
				    gdouble         *top,
				    gdouble         *bottom,
				    gdouble         *left,
				    gdouble         *right)
{
  if (context->has_hard_margins)
    {
      *top    = context->hard_margin_top / context->pixels_per_unit_y;
      *bottom = context->hard_margin_bottom / context->pixels_per_unit_y;
      *left   = context->hard_margin_left / context->pixels_per_unit_x;
      *right  = context->hard_margin_right / context->pixels_per_unit_x;
    }

  return context->has_hard_margins;
}

/**
 * btk_print_context_set_hard_margins:
 * @context: a #BtkPrintContext
 * @top: top hardware printer margin
 * @bottom: bottom hardware printer margin
 * @left: left hardware printer margin
 * @right: right hardware printer margin
 *
 * set the hard margins in pixel coordinates
 */
void
_btk_print_context_set_hard_margins (BtkPrintContext *context,
				     gdouble          top,
				     gdouble          bottom,
				     gdouble          left,
				     gdouble          right)
{
  context->hard_margin_top    = top;
  context->hard_margin_bottom = bottom;
  context->hard_margin_left   = left;
  context->hard_margin_right  = right;
  context->has_hard_margins   = TRUE;
}

/**
 * btk_print_context_get_bango_fontmap:
 * @context: a #BtkPrintContext
 *
 * Returns a #BangoFontMap that is suitable for use
 * with the #BtkPrintContext.
 *
 * Return value: (transfer none): the font map of @context
 *
 * Since: 2.10
 */
BangoFontMap *
btk_print_context_get_bango_fontmap (BtkPrintContext *context)
{
  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), NULL);

  return _btk_print_context_get_fontmap (context);
}

/**
 * btk_print_context_create_bango_context:
 * @context: a #BtkPrintContext
 *
 * Creates a new #BangoContext that can be used with the
 * #BtkPrintContext.
 *
 * Return value: (transfer full): a new Bango context for @context
 * 
 * Since: 2.10
 */
BangoContext *
btk_print_context_create_bango_context (BtkPrintContext *context)
{
  BangoContext *bango_context;
  bairo_font_options_t *options;

  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), NULL);
  
  bango_context = bango_font_map_create_context (_btk_print_context_get_fontmap (context));

  options = bairo_font_options_create ();
  bairo_font_options_set_hint_metrics (options, BAIRO_HINT_METRICS_OFF);
  bango_bairo_context_set_font_options (bango_context, options);
  bairo_font_options_destroy (options);
  
  /* We use the unit-scaled resolution, as we still want 
   * fonts given in points to work 
   */
  bango_bairo_context_set_resolution (bango_context,
				      context->surface_dpi_y / context->pixels_per_unit_y);
  return bango_context;
}

/**
 * btk_print_context_create_bango_layout:
 * @context: a #BtkPrintContext
 *
 * Creates a new #BangoLayout that is suitable for use
 * with the #BtkPrintContext.
 * 
 * Return value: (transfer full): a new Bango layout for @context
 *
 * Since: 2.10
 */
BangoLayout *
btk_print_context_create_bango_layout (BtkPrintContext *context)
{
  BangoContext *bango_context;
  BangoLayout *layout;

  g_return_val_if_fail (BTK_IS_PRINT_CONTEXT (context), NULL);

  bango_context = btk_print_context_create_bango_context (context);
  layout = bango_layout_new (bango_context);

  bango_bairo_update_context (context->cr, bango_context);
  g_object_unref (bango_context);

  return layout;
}


#define __BTK_PRINT_CONTEXT_C__
#include "btkaliasdef.c"
