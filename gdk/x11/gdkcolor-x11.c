/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <time.h>

#include "bdkcolor.h"
#include "bdkinternals.h"
#include "bdkx.h"
#include "bdkprivate-x11.h"
#include "bdkscreen-x11.h"
#include "bdkalias.h"

typedef struct _BdkColormapPrivateX11  BdkColormapPrivateX11;

struct _BdkColormapPrivateX11
{
  BdkScreen *screen;
  Colormap xcolormap;
  gint private_val;

  GHashTable *hash;
  BdkColorInfo *info;
  time_t last_sync_time;

  gboolean foreign;
};

#define BDK_COLORMAP_PRIVATE_DATA(cmap) ((BdkColormapPrivateX11 *) BDK_COLORMAP (cmap)->windowing_data)

static gint     bdk_colormap_match_color (BdkColormap *cmap,
					  BdkColor    *color,
					  const gchar *available);
static void     bdk_colormap_add         (BdkColormap *cmap);
static void     bdk_colormap_remove      (BdkColormap *cmap);

static BdkColormap *bdk_colormap_lookup   (BdkScreen   *screen,
					   Colormap     xcolormap);

static guint    bdk_colormap_hash        (Colormap    *cmap);
static gboolean bdk_colormap_equal       (Colormap    *a,
					  Colormap    *b);
static void     bdk_colormap_sync        (BdkColormap *colormap,
                                          gboolean     force);

static void bdk_colormap_finalize   (GObject              *object);

G_DEFINE_TYPE (BdkColormap, bdk_colormap, G_TYPE_OBJECT)

static void
bdk_colormap_init (BdkColormap *colormap)
{
  BdkColormapPrivateX11 *private;

  private = G_TYPE_INSTANCE_GET_PRIVATE (colormap, BDK_TYPE_COLORMAP, 
					 BdkColormapPrivateX11);

  colormap->windowing_data = private;
  
  private->screen = NULL;
  private->hash = NULL;
  private->last_sync_time = 0;
  private->info = NULL;

  colormap->size = 0;
  colormap->colors = NULL;
}

static void
bdk_colormap_class_init (BdkColormapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bdk_colormap_finalize;

  g_type_class_add_private (object_class, sizeof (BdkColormapPrivateX11));
}

static void
bdk_colormap_finalize (GObject *object)
{
  BdkColormap *colormap = BDK_COLORMAP (object);
  BdkColormapPrivateX11 *private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  bdk_colormap_remove (colormap);

  if (!private->screen->closed && !private->foreign)
    XFreeColormap (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap);

  if (private->hash)
    g_hash_table_destroy (private->hash);
  
  g_free (private->info);
  g_free (colormap->colors);
  
  G_OBJECT_CLASS (bdk_colormap_parent_class)->finalize (object);
}

/**
 * bdk_colormap_new:
 * @visual: a #BdkVisual.
 * @allocate: if %TRUE, the newly created colormap will be
 * a private colormap, and all colors in it will be
 * allocated for the applications use.
 * 
 * Creates a new colormap for the given visual.
 * 
 * Return value: the new #BdkColormap.
 **/
BdkColormap*
bdk_colormap_new (BdkVisual *visual,
		  gboolean   allocate)
{
  BdkColormap *colormap;
  BdkColormapPrivateX11 *private;
  Visual *xvisual;
  Display *xdisplay;
  Window xrootwin;
  int size;
  int i;

  /* FIXME when object properties settle down, there needs to be some
   * kind of default construction (and construct-only arguments)
   */
  
  g_return_val_if_fail (BDK_IS_VISUAL (visual), NULL);

  colormap = g_object_new (BDK_TYPE_COLORMAP, NULL);
  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  colormap->visual = visual;
  private->screen = bdk_visual_get_screen (visual);
  
  xvisual = ((BdkVisualPrivate*) visual)->xvisual;
  xdisplay = BDK_SCREEN_XDISPLAY (private->screen);
  xrootwin = BDK_SCREEN_XROOTWIN (private->screen);

  colormap->size = visual->colormap_size;

  switch (visual->type)
    {
    case BDK_VISUAL_GRAYSCALE:
    case BDK_VISUAL_PSEUDO_COLOR:
      private->info = g_new0 (BdkColorInfo, colormap->size);
      colormap->colors = g_new (BdkColor, colormap->size);
      
      private->hash = g_hash_table_new ((GHashFunc) bdk_color_hash,
					(GEqualFunc) bdk_color_equal);
      
      private->private_val = allocate;
      private->xcolormap = XCreateColormap (xdisplay, xrootwin,
					    xvisual, (allocate) ? (AllocAll) : (AllocNone));

      if (allocate)
	{
          BdkVisual *system_visual;
	  XColor *default_colors;
          gint n_default_colors;

          system_visual = bdk_screen_get_system_visual (private->screen);
	  n_default_colors = MIN (system_visual->colormap_size, colormap->size);

 	  default_colors = g_new (XColor, colormap->size);

	  for (i = 0; i < n_default_colors; i++)
	    default_colors[i].pixel = i;
	  
	  XQueryColors (xdisplay,
			DefaultColormapOfScreen (BDK_SCREEN_X11 (private->screen)->xscreen),
			default_colors, n_default_colors);

	  for (i = 0; i < n_default_colors; i++)
	    {
	      colormap->colors[i].pixel = default_colors[i].pixel;
	      colormap->colors[i].red = default_colors[i].red;
	      colormap->colors[i].green = default_colors[i].green;
	      colormap->colors[i].blue = default_colors[i].blue;
	    }

	  bdk_colormap_change (colormap, n_default_colors);
	  
	  g_free (default_colors);
	}
      break;

    case BDK_VISUAL_DIRECT_COLOR:
      private->private_val = TRUE;
      private->xcolormap = XCreateColormap (xdisplay, xrootwin,
					    xvisual, AllocAll);
      colormap->colors = g_new (BdkColor, colormap->size);

      size = 1 << visual->red_prec;
      for (i = 0; i < size; i++)
	colormap->colors[i].red = i * 65535 / (size - 1);

      size = 1 << visual->green_prec;
      for (i = 0; i < size; i++)
	colormap->colors[i].green = i * 65535 / (size - 1);

      size = 1 << visual->blue_prec;
      for (i = 0; i < size; i++)
	colormap->colors[i].blue = i * 65535 / (size - 1);

      bdk_colormap_change (colormap, colormap->size);
      break;

    case BDK_VISUAL_STATIC_GRAY:
    case BDK_VISUAL_STATIC_COLOR:
      private->private_val = FALSE;
      private->xcolormap = XCreateColormap (xdisplay, xrootwin,
					    xvisual, AllocNone);
      
      colormap->colors = g_new (BdkColor, colormap->size);
      bdk_colormap_sync (colormap, TRUE);
      break;
      
    case BDK_VISUAL_TRUE_COLOR:
      private->private_val = FALSE;
      private->xcolormap = XCreateColormap (xdisplay, xrootwin,
					    xvisual, AllocNone);
      break;
    }

  bdk_colormap_add (colormap);

  return colormap;
}

static void
bdk_colormap_sync_palette (BdkColormap *colormap)
{
  BdkColormapPrivateX11 *private = BDK_COLORMAP_PRIVATE_DATA (colormap);
  XColor *xpalette;
  gint nlookup;
  gint i;

  nlookup = 0;
  xpalette = g_new (XColor, colormap->size);
  
  for (i = 0; i < colormap->size; i++)
    {
      if (!private->info || private->info[i].ref_count == 0)
	{
	  xpalette[nlookup].pixel = i;
	  xpalette[nlookup].red = 0;
	  xpalette[nlookup].green = 0;
	  xpalette[nlookup].blue = 0;
	  nlookup++;
	}
    }

  XQueryColors (BDK_SCREEN_XDISPLAY (private->screen),
		private->xcolormap, xpalette, nlookup);
  
  for (i = 0; i < nlookup; i++)
    {
      gulong pixel = xpalette[i].pixel;
      colormap->colors[pixel].pixel = pixel;
      colormap->colors[pixel].red = xpalette[i].red;
      colormap->colors[pixel].green = xpalette[i].green;
      colormap->colors[pixel].blue = xpalette[i].blue;
    }
  
  g_free (xpalette);
}

static void
bdk_colormap_sync_direct_color (BdkColormap *colormap)
{
  BdkColormapPrivateX11 *private = BDK_COLORMAP_PRIVATE_DATA (colormap);
  BdkVisual *visual = colormap->visual;
  XColor *xpalette;
  gint i;

  xpalette = g_new (XColor, colormap->size);
  
  for (i = 0; i < colormap->size; i++)
    {
      xpalette[i].pixel =
	(((i << visual->red_shift)   & visual->red_mask)   |
	 ((i << visual->green_shift) & visual->green_mask) |
	 ((i << visual->blue_shift)  & visual->blue_mask));
    }

  XQueryColors (BDK_SCREEN_XDISPLAY (private->screen),
		private->xcolormap, xpalette, colormap->size);
  
  for (i = 0; i < colormap->size; i++)
    {
      colormap->colors[i].pixel = xpalette[i].pixel;
      colormap->colors[i].red = xpalette[i].red;
      colormap->colors[i].green = xpalette[i].green;
      colormap->colors[i].blue = xpalette[i].blue;
    }
  
  g_free (xpalette);
}

#define MIN_SYNC_TIME 2

static void
bdk_colormap_sync (BdkColormap *colormap,
		   gboolean     force)
{
  time_t current_time;
  BdkColormapPrivateX11 *private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  if (private->screen->closed)
    return;

  current_time = time (NULL);
  if (!force && ((current_time - private->last_sync_time) < MIN_SYNC_TIME))
    return;

  private->last_sync_time = current_time;

  if (colormap->visual->type == BDK_VISUAL_DIRECT_COLOR)
    bdk_colormap_sync_direct_color (colormap);
  else
    bdk_colormap_sync_palette (colormap);
}
		   
/**
 * bdk_screen_get_system_colormap:
 * @screen: a #BdkScreen
 *
 * Gets the system's default colormap for @screen
 *
 * Returns: (transfer none): the default colormap for @screen.
 *
 * Since: 2.2
 */
BdkColormap *
bdk_screen_get_system_colormap (BdkScreen *screen)
{
  BdkColormap *colormap = NULL;
  BdkColormapPrivateX11 *private;
  BdkScreenX11 *screen_x11;

  g_return_val_if_fail (BDK_IS_SCREEN (screen), NULL);
  screen_x11 = BDK_SCREEN_X11 (screen);

  if (screen_x11->system_colormap)
    return screen_x11->system_colormap;

  colormap = g_object_new (BDK_TYPE_COLORMAP, NULL);
  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  private->screen = screen;
  colormap->visual = bdk_screen_get_system_visual (screen);
  
  private->xcolormap = DefaultColormapOfScreen (screen_x11->xscreen);
  private->private_val = FALSE;

  private->hash = NULL;
  private->last_sync_time = 0;
  private->info = NULL;

  colormap->colors = NULL;
  colormap->size = colormap->visual->colormap_size;

  switch (colormap->visual->type)
    {
    case BDK_VISUAL_GRAYSCALE:
    case BDK_VISUAL_PSEUDO_COLOR:
      private->info = g_new0 (BdkColorInfo, colormap->size);
      private->hash = g_hash_table_new ((GHashFunc) bdk_color_hash,
					(GEqualFunc) bdk_color_equal);
      /* Fall through */
    case BDK_VISUAL_STATIC_GRAY:
    case BDK_VISUAL_STATIC_COLOR:
    case BDK_VISUAL_DIRECT_COLOR:
      colormap->colors = g_new (BdkColor, colormap->size);
      bdk_colormap_sync (colormap, TRUE);
      
    case BDK_VISUAL_TRUE_COLOR:
      break;
    }
  
  bdk_colormap_add (colormap);
  screen_x11->system_colormap = colormap;
  
  return colormap;
}

/**
 * bdk_colormap_get_system_size:
 * 
 * Returns the size of the system's default colormap.
 * (See the description of struct #BdkColormap for an
 * explanation of the size of a colormap.)
 * 
 * Return value: the size of the system's default colormap.
 **/
gint
bdk_colormap_get_system_size (void)
{
  return DisplayCells (BDK_SCREEN_XDISPLAY (bdk_screen_get_default()),
		       BDK_SCREEN_X11 (bdk_screen_get_default())->screen_num);
}

/**
 * bdk_colormap_change:
 * @colormap: a #BdkColormap.
 * @ncolors: the number of colors to change.
 * 
 * Changes the value of the first @ncolors in a private colormap
 * to match the values in the <structfield>colors</structfield>
 * array in the colormap. This function is obsolete and
 * should not be used. See bdk_color_change().
 **/
void
bdk_colormap_change (BdkColormap *colormap,
		     gint         ncolors)
{
  BdkColormapPrivateX11 *private;
  BdkVisual *visual;
  XColor *palette;
  Display *xdisplay;
  gint shift;
  int max_colors;
  int size;
  int i;

  g_return_if_fail (BDK_IS_COLORMAP (colormap));

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if (private->screen->closed)
    return;

  xdisplay = BDK_SCREEN_XDISPLAY (private->screen);
  palette = g_new (XColor, ncolors);

  switch (colormap->visual->type)
    {
    case BDK_VISUAL_GRAYSCALE:
    case BDK_VISUAL_PSEUDO_COLOR:
      for (i = 0; i < ncolors; i++)
	{
	  palette[i].pixel = colormap->colors[i].pixel;
	  palette[i].red = colormap->colors[i].red;
	  palette[i].green = colormap->colors[i].green;
	  palette[i].blue = colormap->colors[i].blue;
	  palette[i].flags = DoRed | DoGreen | DoBlue;
	}

      XStoreColors (xdisplay, private->xcolormap, palette, ncolors);
      break;

    case BDK_VISUAL_DIRECT_COLOR:
      visual = colormap->visual;

      shift = visual->red_shift;
      max_colors = 1 << visual->red_prec;
      size = (ncolors < max_colors) ? (ncolors) : (max_colors);

      for (i = 0; i < size; i++)
	{
	  palette[i].pixel = i << shift;
	  palette[i].red = colormap->colors[i].red;
	  palette[i].flags = DoRed;
	}

      XStoreColors (xdisplay, private->xcolormap, palette, size);

      shift = visual->green_shift;
      max_colors = 1 << visual->green_prec;
      size = (ncolors < max_colors) ? (ncolors) : (max_colors);

      for (i = 0; i < size; i++)
	{
	  palette[i].pixel = i << shift;
	  palette[i].green = colormap->colors[i].green;
	  palette[i].flags = DoGreen;
	}

      XStoreColors (xdisplay, private->xcolormap, palette, size);

      shift = visual->blue_shift;
      max_colors = 1 << visual->blue_prec;
      size = (ncolors < max_colors) ? (ncolors) : (max_colors);

      for (i = 0; i < size; i++)
	{
	  palette[i].pixel = i << shift;
	  palette[i].blue = colormap->colors[i].blue;
	  palette[i].flags = DoBlue;
	}

      XStoreColors (xdisplay, private->xcolormap, palette, size);
      break;

    default:
      break;
    }

  g_free (palette);
}

/**
 * bdk_colors_alloc:
 * @colormap: a #BdkColormap.
 * @contiguous: if %TRUE, the colors should be allocated
 *    in contiguous color cells.
 * @planes: an array in which to store the plane masks.
 * @nplanes: the number of planes to allocate. (Or zero,
 *    to indicate that the color allocation should not be planar.)
 * @pixels: an array into which to store allocated pixel values.
 * @npixels: the number of pixels in each plane to allocate.
 * 
 * Allocates colors from a colormap. This function
 * is obsolete. See bdk_colormap_alloc_colors().
 * For full documentation of the fields, see 
 * the Xlib documentation for <function>XAllocColorCells()</function>.
 * 
 * Return value: %TRUE if the allocation was successful
 **/
gboolean
bdk_colors_alloc (BdkColormap   *colormap,
		  gboolean       contiguous,
		  gulong        *planes,
		  gint           nplanes,
		  gulong        *pixels,
		  gint           npixels)
{
  BdkColormapPrivateX11 *private;
  gint return_val;
  gint i;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), FALSE);

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if (private->screen->closed)
    return FALSE;

  return_val = XAllocColorCells (BDK_SCREEN_XDISPLAY (private->screen),
				 private->xcolormap,contiguous, planes,
				 nplanes, pixels, npixels);
  if (return_val)
    {
      for (i = 0; i < npixels; i++)
	{
	  private->info[pixels[i]].ref_count++;
	  private->info[pixels[i]].flags |= BDK_COLOR_WRITEABLE;
	}
    }

  return return_val != 0;
}

/* This is almost identical to bdk_colormap_free_colors.
 * Keep them in sync!
 */


/**
 * bdk_colors_free:
 * @colormap: a #BdkColormap.
 * @pixels: the pixel values of the colors to free.
 * @npixels: the number of values in @pixels.
 * @planes: the plane masks for all planes to free, OR'd together.
 * 
 * Frees colors allocated with bdk_colors_alloc(). This
 * function is obsolete. See bdk_colormap_free_colors().
 **/
void
bdk_colors_free (BdkColormap *colormap,
		 gulong      *pixels,
		 gint         npixels,
		 gulong       planes)
{
  BdkColormapPrivateX11 *private;
  gulong *pixels_to_free;
  gint npixels_to_free = 0;
  gint i;

  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (pixels != NULL);

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if ((colormap->visual->type != BDK_VISUAL_PSEUDO_COLOR) &&
      (colormap->visual->type != BDK_VISUAL_GRAYSCALE))
    return;
  
  pixels_to_free = g_new (gulong, npixels);

  for (i = 0; i < npixels; i++)
    {
      gulong pixel = pixels[i];
      
      if (private->info[pixel].ref_count)
	{
	  private->info[pixel].ref_count--;

	  if (private->info[pixel].ref_count == 0)
	    {
	      pixels_to_free[npixels_to_free++] = pixel;
	      if (!(private->info[pixel].flags & BDK_COLOR_WRITEABLE))
		g_hash_table_remove (private->hash, &colormap->colors[pixel]);
	      private->info[pixel].flags = 0;
	    }
	}
    }

  if (npixels_to_free && !private->private_val && !private->screen->closed)
    XFreeColors (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap,
		 pixels_to_free, npixels_to_free, planes);
  g_free (pixels_to_free);
}

/* This is almost identical to bdk_colors_free.
 * Keep them in sync!
 */

/**
 * bdk_colormap_free_colors:
 * @colormap: a #BdkColormap.
 * @colors: the colors to free.
 * @n_colors: the number of colors in @colors.
 * 
 * Frees previously allocated colors.
 **/
void
bdk_colormap_free_colors (BdkColormap    *colormap,
			  const BdkColor *colors,
			  gint            n_colors)
{
  BdkColormapPrivateX11 *private;
  gulong *pixels;
  gint npixels = 0;
  gint i;

  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  g_return_if_fail (colors != NULL);

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if ((colormap->visual->type != BDK_VISUAL_PSEUDO_COLOR) &&
      (colormap->visual->type != BDK_VISUAL_GRAYSCALE))
    return;

  pixels = g_new (gulong, n_colors);

  for (i = 0; i < n_colors; i++)
    {
      gulong pixel = colors[i].pixel;
      
      if (private->info[pixel].ref_count)
	{
	  private->info[pixel].ref_count--;

	  if (private->info[pixel].ref_count == 0)
	    {
	      pixels[npixels++] = pixel;
	      if (!(private->info[pixel].flags & BDK_COLOR_WRITEABLE))
		g_hash_table_remove (private->hash, &colormap->colors[pixel]);
	      private->info[pixel].flags = 0;
	    }
	}
    }

  if (npixels && !private->private_val && !private->screen->closed)
    XFreeColors (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap,
		 pixels, npixels, 0);

  g_free (pixels);
}

/********************
 * Color allocation *
 ********************/

/* Try to allocate a single color using XAllocColor. If it succeeds,
 * cache the result in our colormap, and store in ret.
 */
static gboolean 
bdk_colormap_alloc1 (BdkColormap    *colormap,
		     const BdkColor *color,
		     BdkColor       *ret)
{
  BdkColormapPrivateX11 *private;
  XColor xcolor;

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  xcolor.red = color->red;
  xcolor.green = color->green;
  xcolor.blue = color->blue;
  xcolor.pixel = color->pixel;
  xcolor.flags = DoRed | DoGreen | DoBlue;

  if (XAllocColor (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap, &xcolor))
    {
      ret->pixel = xcolor.pixel;
      ret->red = xcolor.red;
      ret->green = xcolor.green;
      ret->blue = xcolor.blue;
      
      if (ret->pixel < colormap->size)
	{
	  if (private->info[ret->pixel].ref_count) /* got a duplicate */
	    {
	      XFreeColors (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap,
			   &xcolor.pixel, 1, 0);
	      private->info[ret->pixel].ref_count++;
	    }
	  else
	    {
	      colormap->colors[ret->pixel] = *color;
	      colormap->colors[ret->pixel].pixel = ret->pixel;
	      private->info[ret->pixel].ref_count = 1;

	      g_hash_table_insert (private->hash,
				   &colormap->colors[ret->pixel],
				   &colormap->colors[ret->pixel]);
	    }
	}
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}

static gint
bdk_colormap_alloc_colors_writeable (BdkColormap *colormap,
				     BdkColor    *colors,
				     gint         ncolors,
				     gboolean     writeable,
				     gboolean     best_match,
				     gboolean    *success)
{
  BdkColormapPrivateX11 *private;
  gulong *pixels;
  Status status;
  gint i, index;

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if (private->private_val)
    {
      index = 0;
      for (i = 0; i < ncolors; i++)
	{
	  while ((index < colormap->size) && (private->info[index].ref_count != 0))
	    index++;
	  
	  if (index < colormap->size)
	    {
	      colors[i].pixel = index;
	      success[i] = TRUE;
	      private->info[index].ref_count++;
	      private->info[i].flags |= BDK_COLOR_WRITEABLE;
	    }
	  else
	    break;
	}
      return ncolors - i;
    }
  else
    {
      pixels = g_new (gulong, ncolors);
      /* Allocation of a writeable color cells */
      
      status =  XAllocColorCells (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap,
				  FALSE, NULL, 0, pixels, ncolors);
      if (status)
	{
	  for (i = 0; i < ncolors; i++)
	    {
	      colors[i].pixel = pixels[i];
	      success[i] = TRUE;
	      private->info[pixels[i]].ref_count++;
	      private->info[pixels[i]].flags |= BDK_COLOR_WRITEABLE;
	    }
	}
      
      g_free (pixels);

      return status ? 0 : ncolors; 
    }
}

static gint
bdk_colormap_alloc_colors_private (BdkColormap *colormap,
				   BdkColor    *colors,
				   gint         ncolors,
				   gboolean     writeable,
				   gboolean     best_match,
				   gboolean    *success)
{
  BdkColormapPrivateX11 *private;
  gint i, index;
  XColor *store = g_new (XColor, ncolors);
  gint nstore = 0;
  gint nremaining = 0;
  
  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  /* First, store the colors we have room for */

  index = 0;
  for (i = 0; i < ncolors; i++)
    {
      if (!success[i])
	{
	  while ((index < colormap->size) && (private->info[index].ref_count != 0))
	    index++;

	  if (index < colormap->size)
	    {
	      store[nstore].red = colors[i].red;
	      store[nstore].blue = colors[i].blue;
	      store[nstore].green = colors[i].green;
	      store[nstore].pixel = index;
	      store[nstore].flags = DoRed | DoGreen | DoBlue;
	      nstore++;

	      success[i] = TRUE;
	      colors[i].pixel = index;

	      colormap->colors[index] = colors[i];
	      private->info[index].ref_count++;

	      g_hash_table_insert (private->hash,
				   &colormap->colors[index],
				   &colormap->colors[index]);
	    }
	  else
	    nremaining++;
	}
    }
  
  XStoreColors (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap,
		store, nstore);
  g_free (store);

  if (nremaining > 0 && best_match)
    {
      /* Get best matches for remaining colors */

      gchar *available = g_new (gchar, colormap->size);
      for (i = 0; i < colormap->size; i++)
	available[i] = !(private->info[i].flags & BDK_COLOR_WRITEABLE);

      for (i = 0; i < ncolors; i++)
	{
	  if (!success[i])
	    {
	      index = bdk_colormap_match_color (colormap, 
						&colors[i], 
						available);
	      if (index != -1)
		{
		  colors[i] = colormap->colors[index];
		  private->info[index].ref_count++;

		  success[i] = TRUE;
		  nremaining--;
		}
	    }
	}
      g_free (available);
    }

  return nremaining;
}

static gint
bdk_colormap_alloc_colors_shared (BdkColormap *colormap,
				  BdkColor    *colors,
				  gint         ncolors,
				  gboolean     writeable,
				  gboolean     best_match,
				  gboolean    *success)
{
  BdkColormapPrivateX11 *private;
  gint i, index;
  gint nremaining = 0;
  gint nfailed = 0;

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  for (i = 0; i < ncolors; i++)
    {
      if (!success[i])
	{
	  if (bdk_colormap_alloc1 (colormap, &colors[i], &colors[i]))
	    success[i] = TRUE;
	  else
	    nremaining++;
	}
    }


  if (nremaining > 0 && best_match)
    {
      gchar *available = g_new (gchar, colormap->size);
      for (i = 0; i < colormap->size; i++)
	available[i] = ((private->info[i].ref_count == 0) ||
			!(private->info[i].flags & BDK_COLOR_WRITEABLE));
      bdk_colormap_sync (colormap, FALSE);
      
      while (nremaining > 0)
	{
	  for (i = 0; i < ncolors; i++)
	    {
	      if (!success[i])
		{
		  index = bdk_colormap_match_color (colormap, &colors[i], available);
		  if (index != -1)
		    {
		      if (private->info[index].ref_count)
			{
			  private->info[index].ref_count++;
			  colors[i] = colormap->colors[index];
			  success[i] = TRUE;
			  nremaining--;
			}
		      else
			{
			  if (bdk_colormap_alloc1 (colormap, 
						   &colormap->colors[index],
						   &colors[i]))
			    {
			      success[i] = TRUE;
			      nremaining--;
			      break;
			    }
			  else
			    {
			      available[index] = FALSE;
			    }
			}
		    }
		  else
		    {
		      nfailed++;
		      nremaining--;
		      success[i] = 2; /* flag as permanent failure */
		    }
		}
	    }
	}
      g_free (available);
    }

  /* Change back the values we flagged as permanent failures */
  if (nfailed > 0)
    {
      for (i = 0; i < ncolors; i++)
	if (success[i] == 2)
	  success[i] = FALSE;
      nremaining = nfailed;
    }
  
  return nremaining;
}

static gint
bdk_colormap_alloc_colors_pseudocolor (BdkColormap *colormap,
				       BdkColor    *colors,
				       gint         ncolors,
				       gboolean     writeable,
				       gboolean     best_match,
				       gboolean    *success)
{
  BdkColormapPrivateX11 *private;
  BdkColor *lookup_color;
  gint i;
  gint nremaining = 0;

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  /* Check for an exact match among previously allocated colors */

  for (i = 0; i < ncolors; i++)
    {
      if (!success[i])
	{
	  lookup_color = g_hash_table_lookup (private->hash, &colors[i]);
	  if (lookup_color)
	    {
	      private->info[lookup_color->pixel].ref_count++;
	      colors[i].pixel = lookup_color->pixel;
	      success[i] = TRUE;
	    }
	  else
	    nremaining++;
	}
    }

  /* If that failed, we try to allocate a new color, or approxmiate
   * with what we can get if best_match is TRUE.
   */
  if (nremaining > 0)
    {
      if (private->private_val)
	return bdk_colormap_alloc_colors_private (colormap, colors, ncolors, writeable, best_match, success);
      else
	return bdk_colormap_alloc_colors_shared (colormap, colors, ncolors, writeable, best_match, success);
    }
  else
    return 0;
}

/**
 * bdk_colormap_alloc_colors:
 * @colormap: a #BdkColormap.
 * @colors: The color values to allocate. On return, the pixel
 *    values for allocated colors will be filled in.
 * @n_colors: The number of colors in @colors.
 * @writeable: If %TRUE, the colors are allocated writeable
 *    (their values can later be changed using bdk_color_change()).
 *    Writeable colors cannot be shared between applications.
 * @best_match: If %TRUE, BDK will attempt to do matching against
 *    existing colors if the colors cannot be allocated as requested.
 * @success: An array of length @ncolors. On return, this
 *   indicates whether the corresponding color in @colors was
 *   successfully allocated or not.
 * 
 * Allocates colors from a colormap.
 * 
 * Return value: The number of colors that were not successfully 
 * allocated.
 **/
gint
bdk_colormap_alloc_colors (BdkColormap *colormap,
			   BdkColor    *colors,
			   gint         n_colors,
			   gboolean     writeable,
			   gboolean     best_match,
			   gboolean    *success)
{
  BdkColormapPrivateX11 *private;
  BdkVisual *visual;
  gint i;
  gint nremaining = 0;
  XColor xcolor;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), n_colors);
  g_return_val_if_fail (colors != NULL, n_colors);
  g_return_val_if_fail (success != NULL, n_colors);

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if (private->screen->closed)
    return n_colors;

  for (i = 0; i < n_colors; i++)
    success[i] = FALSE;

  switch (colormap->visual->type)
    {
    case BDK_VISUAL_PSEUDO_COLOR:
    case BDK_VISUAL_GRAYSCALE:
      if (writeable)
	return bdk_colormap_alloc_colors_writeable (colormap, colors, n_colors,
						    writeable, best_match, success);
      else
	return bdk_colormap_alloc_colors_pseudocolor (colormap, colors, n_colors,
						    writeable, best_match, success);
      break;

    case BDK_VISUAL_DIRECT_COLOR:
    case BDK_VISUAL_TRUE_COLOR:
      visual = colormap->visual;

      for (i = 0; i < n_colors; i++)
	{
	  /* If bits not used for color are used for something other than padding,
	   * it's likely alpha, so we set them to 1s.
	   */
	  guint padding, unused;

	  /* Shifting by >= width-of-type isn't defined in C */
	  if (visual->depth >= 32)
	    padding = 0;
	  else
	    padding = ((~(guint32)0)) << visual->depth;
	  
	  unused = ~ (visual->red_mask | visual->green_mask | visual->blue_mask | padding);
	  
	  colors[i].pixel = (unused +
			     ((colors[i].red >> (16 - visual->red_prec)) << visual->red_shift) +
			     ((colors[i].green >> (16 - visual->green_prec)) << visual->green_shift) +
			     ((colors[i].blue >> (16 - visual->blue_prec)) << visual->blue_shift));
	  success[i] = TRUE;
	}
      break;
    case BDK_VISUAL_STATIC_GRAY:
    case BDK_VISUAL_STATIC_COLOR:
      for (i = 0; i < n_colors; i++)
	{
	  xcolor.red = colors[i].red;
	  xcolor.green = colors[i].green;
	  xcolor.blue = colors[i].blue;
	  xcolor.pixel = colors[i].pixel;
	  xcolor.flags = DoRed | DoGreen | DoBlue;

	  if (XAllocColor (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap, &xcolor))
	    {
	      colors[i].pixel = xcolor.pixel;
	      success[i] = TRUE;
	    }
	  else
	    nremaining++;
	}
      break;
    }
  return nremaining;
}

/**
 * bdk_colormap_query_color:
 * @colormap: a #BdkColormap
 * @pixel: pixel value in hardware display format
 * @result: #BdkColor with red, green, blue fields initialized
 * 
 * Locates the RGB color in @colormap corresponding to the given
 * hardware pixel @pixel. @pixel must be a valid pixel in the
 * colormap; it's a programmer error to call this function with a
 * pixel which is not in the colormap. Hardware pixels are normally
 * obtained from bdk_colormap_alloc_colors(), or from a #BdkImage. (A
 * #BdkImage contains image data in hardware format, a #BdkPixbuf
 * contains image data in a canonical 24-bit RGB format.)
 *
 * This function is rarely useful; it's used for example to
 * implement the eyedropper feature in #BtkColorSelection.
 * 
 **/
void
bdk_colormap_query_color (BdkColormap *colormap,
			  gulong       pixel,
			  BdkColor    *result)
{
  XColor xcolor;
  BdkVisual *visual;
  BdkColormapPrivateX11 *private;
  
  g_return_if_fail (BDK_IS_COLORMAP (colormap));
  
  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  visual = bdk_colormap_get_visual (colormap);

  switch (visual->type) {
  case BDK_VISUAL_DIRECT_COLOR:
  case BDK_VISUAL_TRUE_COLOR:
    result->red = 65535. * (double)((pixel & visual->red_mask) >> visual->red_shift) / ((1 << visual->red_prec) - 1);
    result->green = 65535. * (double)((pixel & visual->green_mask) >> visual->green_shift) / ((1 << visual->green_prec) - 1);
    result->blue = 65535. * (double)((pixel & visual->blue_mask) >> visual->blue_shift) / ((1 << visual->blue_prec) - 1);
    break;
  case BDK_VISUAL_STATIC_GRAY:
  case BDK_VISUAL_GRAYSCALE:
    result->red = result->green = result->blue = 65535. * (double)pixel/((1<<visual->depth) - 1);
    break;
  case BDK_VISUAL_STATIC_COLOR:
    xcolor.pixel = pixel;
    if (!private->screen->closed)
      {
	XQueryColor (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap, &xcolor);
	result->red = xcolor.red;
	result->green = xcolor.green;
	result->blue =  xcolor.blue;
      }
    else
      result->red = result->green = result->blue = 0;
    break;
  case BDK_VISUAL_PSEUDO_COLOR:
    g_return_if_fail (pixel < colormap->size);
    result->red = colormap->colors[pixel].red;
    result->green = colormap->colors[pixel].green;
    result->blue = colormap->colors[pixel].blue;
    break;
  default:
    g_assert_not_reached ();
    break;
  }
}

/**
 * bdk_color_change:
 * @colormap: a #BdkColormap.
 * @color: a #BdkColor, with the color to change
 * in the <structfield>pixel</structfield> field,
 * and the new value in the remaining fields.
 * 
 * Changes the value of a color that has already
 * been allocated. If @colormap is not a private
 * colormap, then the color must have been allocated
 * using bdk_colormap_alloc_colors() with the 
 * @writeable set to %TRUE.
 * 
 * Return value: %TRUE if the color was successfully changed.
 **/
gboolean
bdk_color_change (BdkColormap *colormap,
		  BdkColor    *color)
{
  BdkColormapPrivateX11 *private;
  XColor xcolor;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  xcolor.pixel = color->pixel;
  xcolor.red = color->red;
  xcolor.green = color->green;
  xcolor.blue = color->blue;
  xcolor.flags = DoRed | DoGreen | DoBlue;

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);
  if (!private->screen->closed)
    XStoreColor (BDK_SCREEN_XDISPLAY (private->screen), private->xcolormap, &xcolor);

  return TRUE;
}

/**
 * bdk_x11_colormap_foreign_new:
 * @visual: a #BdkVisual
 * @xcolormap: The XID of a colormap with visual @visual
 * 
 * If xcolormap refers to a colormap previously known to BTK+,
 * returns a new reference to the existing #BdkColormap object,
 * otherwise creates a new BdkColormap object and returns that
 *
 * Return value: the #BdkColormap object for @xcolormap.
 *   Free with g_object_unref(). Note that for colormap created
 *   with bdk_x11_colormap_foreign_new(), unref'ing the last
 *   reference to the object will only free the #BdkColoramp
 *   object and not call XFreeColormap()
 *
 * Since: 2.2
 **/
BdkColormap *
bdk_x11_colormap_foreign_new (BdkVisual *visual,
			      Colormap   xcolormap)
{
  BdkColormap *colormap;
  BdkScreen *screen;
  BdkColormapPrivateX11 *private;
  
  g_return_val_if_fail (BDK_IS_VISUAL (visual), NULL);
  g_return_val_if_fail (xcolormap != None, NULL);

  screen = bdk_visual_get_screen (visual);
  
  if (xcolormap == DefaultColormap (BDK_SCREEN_XDISPLAY (screen),
				    BDK_SCREEN_XNUMBER (screen)))
    return g_object_ref (bdk_screen_get_system_colormap (screen));

  colormap = bdk_colormap_lookup (screen, xcolormap);
  if (colormap)
    return g_object_ref (colormap);

  colormap = g_object_new (BDK_TYPE_COLORMAP, NULL);
  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  colormap->visual = visual;

  private->screen = screen;
  private->xcolormap = xcolormap;
  private->private_val = FALSE;
  private->foreign = TRUE;

  colormap->size = visual->colormap_size;

  switch (colormap->visual->type)
    {
    case BDK_VISUAL_GRAYSCALE:
    case BDK_VISUAL_PSEUDO_COLOR:
      private->info = g_new0 (BdkColorInfo, colormap->size);
      private->hash = g_hash_table_new ((GHashFunc) bdk_color_hash,
					(GEqualFunc) bdk_color_equal);
      /* Fall through */
    case BDK_VISUAL_STATIC_GRAY:
    case BDK_VISUAL_STATIC_COLOR:
    case BDK_VISUAL_DIRECT_COLOR:
      colormap->colors = g_new (BdkColor, colormap->size);
      bdk_colormap_sync (colormap, TRUE);
      
    case BDK_VISUAL_TRUE_COLOR:
      break;
    }

  bdk_colormap_add (colormap);

  return colormap;
  
}

/**
 * bdkx_colormap_get:
 * @xcolormap: the XID of a colormap for the default screen.
 * 
 * Returns a #BdkColormap corresponding to a X colormap;
 * this function only works if the colormap is already
 * known to BTK+ (a colormap created by BTK+ or the default
 * colormap for the screen), since BTK+ 
 *
 * Always use bdk_x11_colormap_foreign_new() instead.
 *
 * Return value: the existing #BdkColormap object if it was
 *  already known to BTK+, otherwise warns and return
 *  %NULL.
 **/
BdkColormap*
bdkx_colormap_get (Colormap xcolormap)
{
  BdkScreen *screen = bdk_screen_get_default ();
  BdkColormap *colormap;

  if (xcolormap == DefaultColormap (BDK_SCREEN_XDISPLAY (screen),
				    BDK_SCREEN_XNUMBER (screen)))
    return g_object_ref (bdk_screen_get_system_colormap (screen));

  colormap = bdk_colormap_lookup (screen, xcolormap);
  if (colormap)
    return g_object_ref (colormap);

  g_warning ("Colormap passed to bdkx_colormap_get\n"
	     "does not previously exist");

  return NULL;
}

static gint
bdk_colormap_match_color (BdkColormap *cmap,
			  BdkColor    *color,
			  const gchar *available)
{
  BdkColor *colors;
  guint sum, max;
  gint rdiff, gdiff, bdiff;
  gint i, index;

  colors = cmap->colors;
  max = 3 * (65536);
  index = -1;

  for (i = 0; i < cmap->size; i++)
    {
      if ((!available) || (available && available[i]))
	{
	  rdiff = (color->red - colors[i].red);
	  gdiff = (color->green - colors[i].green);
	  bdiff = (color->blue - colors[i].blue);

	  sum = ABS (rdiff) + ABS (gdiff) + ABS (bdiff);

	  if (sum < max)
	    {
	      index = i;
	      max = sum;
	    }
	}
    }

  return index;
}


static BdkColormap*
bdk_colormap_lookup (BdkScreen *screen,
		     Colormap   xcolormap)
{
  BdkScreenX11 *screen_x11 = BDK_SCREEN_X11 (screen);

  if (screen_x11->colormap_hash)
    return g_hash_table_lookup (screen_x11->colormap_hash, &xcolormap);
  else
    return NULL;
}

static void
bdk_colormap_add (BdkColormap *cmap)
{
  BdkScreenX11 *screen_x11;
  BdkColormapPrivateX11 *private;

  private = BDK_COLORMAP_PRIVATE_DATA (cmap);
  screen_x11 = BDK_SCREEN_X11 (private->screen);

  if (!screen_x11->colormap_hash)
    screen_x11->colormap_hash = g_hash_table_new ((GHashFunc) bdk_colormap_hash,
						  (GEqualFunc) bdk_colormap_equal);

  g_hash_table_insert (screen_x11->colormap_hash, &private->xcolormap, cmap);
}

static void
bdk_colormap_remove (BdkColormap *cmap)
{
  BdkScreenX11 *screen_x11;
  BdkColormapPrivateX11 *private;

  private = BDK_COLORMAP_PRIVATE_DATA (cmap);
  screen_x11 = BDK_SCREEN_X11 (private->screen);

  if (screen_x11->colormap_hash)
    g_hash_table_remove (screen_x11->colormap_hash, &private->xcolormap);
}

static guint
bdk_colormap_hash (Colormap *colormap)
{
  return *colormap;
}

static gboolean
bdk_colormap_equal (Colormap *a,
		    Colormap *b)
{
  return (*a == *b);
}

/**
 * bdk_x11_colormap_get_xdisplay:
 * @colormap: a #BdkColormap.
 * 
 * Returns the display of a #BdkColormap.
 * 
 * Return value: an Xlib <type>Display*</type>.
 **/
Display *
bdk_x11_colormap_get_xdisplay (BdkColormap *colormap)
{
  BdkColormapPrivateX11 *private;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), NULL);

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  return BDK_SCREEN_XDISPLAY (private->screen);
}

/**
 * bdk_x11_colormap_get_xcolormap:
 * @colormap:  a #BdkColormap.
 * 
 * Returns the X colormap belonging to a #BdkColormap.
 * 
 * Return value: an Xlib <type>Colormap</type>.
 **/
Colormap
bdk_x11_colormap_get_xcolormap (BdkColormap *colormap)
{
  BdkColormapPrivateX11 *private;

  g_return_val_if_fail (BDK_IS_COLORMAP (colormap), None);

  private = BDK_COLORMAP_PRIVATE_DATA (colormap);

  if (private->screen->closed)
    return None;
  else
    return private->xcolormap;
}

/**
 * bdk_colormap_get_screen:
 * @cmap: a #BdkColormap
 * 
 * Gets the screen for which this colormap was created.
 * 
 * Return value: the screen for which this colormap was created.
 *
 * Since: 2.2
 **/
BdkScreen *
bdk_colormap_get_screen (BdkColormap *cmap)
{
  g_return_val_if_fail (BDK_IS_COLORMAP (cmap), NULL);

  return  BDK_COLORMAP_PRIVATE_DATA (cmap)->screen;
}

#define __BDK_COLOR_X11_C__
#include "bdkaliasdef.c"
