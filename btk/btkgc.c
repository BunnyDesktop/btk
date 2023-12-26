/* BTK - The GIMP Toolkit
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

#undef BDK_DISABLE_DEPRECATED

#include "btkgc.h"
#include "btkintl.h"
#include "btkalias.h"


typedef struct _BtkGCKey       BtkGCKey;
typedef struct _BtkGCDrawable  BtkGCDrawable;

struct _BtkGCKey
{
  bint depth;
  BdkColormap *colormap;
  BdkGCValues values;
  BdkGCValuesMask mask;
};

struct _BtkGCDrawable
{
  bint depth;
  BdkPixmap *drawable;
};


static void      btk_gc_init             (void);
static BtkGCKey* btk_gc_key_dup          (BtkGCKey      *key);
static void      btk_gc_key_destroy      (BtkGCKey      *key);
static bpointer  btk_gc_new              (bpointer       key);
static void      btk_gc_destroy          (bpointer       value);
static buint     btk_gc_key_hash         (bpointer       key);
static buint     btk_gc_value_hash       (bpointer       value);
static bint      btk_gc_key_equal        (bpointer       a,
					  bpointer       b);
static buint     btk_gc_drawable_hash    (BtkGCDrawable *d);
static bint      btk_gc_drawable_equal   (BtkGCDrawable *a,
					  BtkGCDrawable *b);


static bint initialize = TRUE;
static GCache *gc_cache = NULL;
static GQuark quark_btk_gc_drawable_ht = 0;

BdkGC*
btk_gc_get (bint             depth,
	    BdkColormap     *colormap,
	    BdkGCValues     *values,
	    BdkGCValuesMask  values_mask)
{
  BtkGCKey key;
  BdkGC *gc;

  if (initialize)
    btk_gc_init ();

  key.depth = depth;
  key.colormap = colormap;
  key.values = *values;
  key.mask = values_mask;

  gc = g_cache_insert (gc_cache, &key);

  return gc;
}

void
btk_gc_release (BdkGC *gc)
{
  if (initialize)
    btk_gc_init ();

  g_cache_remove (gc_cache, gc);
}

static void 
free_gc_drawable (bpointer data)
{
  BtkGCDrawable *drawable = data;
  g_object_unref (drawable->drawable);
  g_slice_free (BtkGCDrawable, drawable);
}

static GHashTable*
btk_gc_get_drawable_ht (BdkScreen *screen)
{
  GHashTable *ht = g_object_get_qdata (B_OBJECT (screen), quark_btk_gc_drawable_ht);
  if (!ht)
    {
      ht = g_hash_table_new_full ((GHashFunc) btk_gc_drawable_hash,
				  (GEqualFunc) btk_gc_drawable_equal,
				  NULL, free_gc_drawable);
      g_object_set_qdata_full (B_OBJECT (screen), 
			       quark_btk_gc_drawable_ht, ht, 
			       (GDestroyNotify)g_hash_table_destroy);
    }
  
  return ht;
}

static void
btk_gc_init (void)
{
  initialize = FALSE;

  quark_btk_gc_drawable_ht = g_quark_from_static_string ("btk-gc-drawable-ht");

  gc_cache = g_cache_new ((GCacheNewFunc) btk_gc_new,
			  (GCacheDestroyFunc) btk_gc_destroy,
			  (GCacheDupFunc) btk_gc_key_dup,
			  (GCacheDestroyFunc) btk_gc_key_destroy,
			  (GHashFunc) btk_gc_key_hash,
			  (GHashFunc) btk_gc_value_hash,
			  (GEqualFunc) btk_gc_key_equal);
}

static BtkGCKey*
btk_gc_key_dup (BtkGCKey *key)
{
  BtkGCKey *new_key;

  new_key = g_slice_new (BtkGCKey);

  *new_key = *key;

  return new_key;
}

static void
btk_gc_key_destroy (BtkGCKey *key)
{
  g_slice_free (BtkGCKey, key);
}

static bpointer
btk_gc_new (bpointer key)
{
  BtkGCKey *keyval;
  BtkGCDrawable *drawable;
  BdkGC *gc;
  GHashTable *ht;
  BdkScreen *screen;

  keyval = key;
  screen = bdk_colormap_get_screen (keyval->colormap);
  
  ht = btk_gc_get_drawable_ht (screen);
  drawable = g_hash_table_lookup (ht, &keyval->depth);
  if (!drawable)
    {
      drawable = g_slice_new (BtkGCDrawable);
      drawable->depth = keyval->depth;
      drawable->drawable = bdk_pixmap_new (bdk_screen_get_root_window (screen), 
					   1, 1, drawable->depth);
      g_hash_table_insert (ht, &drawable->depth, drawable);
    }

  gc = bdk_gc_new_with_values (drawable->drawable, &keyval->values, keyval->mask);
  bdk_gc_set_colormap (gc, keyval->colormap);

  return (bpointer) gc;
}

static void
btk_gc_destroy (bpointer value)
{
  g_object_unref (value);
}

static buint
btk_gc_key_hash (bpointer key)
{
  BtkGCKey *keyval;
  buint hash_val;

  keyval = key;
  hash_val = 0;

  if (keyval->mask & BDK_GC_FOREGROUND)
    {
      hash_val += keyval->values.foreground.pixel;
    }
  if (keyval->mask & BDK_GC_BACKGROUND)
    {
      hash_val += keyval->values.background.pixel;
    }
  if (keyval->mask & BDK_GC_FONT)
    {
      hash_val += bdk_font_id (keyval->values.font);
    }
  if (keyval->mask & BDK_GC_FUNCTION)
    {
      hash_val += (bint) keyval->values.function;
    }
  if (keyval->mask & BDK_GC_FILL)
    {
      hash_val += (bint) keyval->values.fill;
    }
  if (keyval->mask & BDK_GC_TILE)
    {
      hash_val += (bintptr) keyval->values.tile;
    }
  if (keyval->mask & BDK_GC_STIPPLE)
    {
      hash_val += (bintptr) keyval->values.stipple;
    }
  if (keyval->mask & BDK_GC_CLIP_MASK)
    {
      hash_val += (bintptr) keyval->values.clip_mask;
    }
  if (keyval->mask & BDK_GC_SUBWINDOW)
    {
      hash_val += (bint) keyval->values.subwindow_mode;
    }
  if (keyval->mask & BDK_GC_TS_X_ORIGIN)
    {
      hash_val += (bint) keyval->values.ts_x_origin;
    }
  if (keyval->mask & BDK_GC_TS_Y_ORIGIN)
    {
      hash_val += (bint) keyval->values.ts_y_origin;
    }
  if (keyval->mask & BDK_GC_CLIP_X_ORIGIN)
    {
      hash_val += (bint) keyval->values.clip_x_origin;
    }
  if (keyval->mask & BDK_GC_CLIP_Y_ORIGIN)
    {
      hash_val += (bint) keyval->values.clip_y_origin;
    }
  if (keyval->mask & BDK_GC_EXPOSURES)
    {
      hash_val += (bint) keyval->values.graphics_exposures;
    }
  if (keyval->mask & BDK_GC_LINE_WIDTH)
    {
      hash_val += (bint) keyval->values.line_width;
    }
  if (keyval->mask & BDK_GC_LINE_STYLE)
    {
      hash_val += (bint) keyval->values.line_style;
    }
  if (keyval->mask & BDK_GC_CAP_STYLE)
    {
      hash_val += (bint) keyval->values.cap_style;
    }
  if (keyval->mask & BDK_GC_JOIN_STYLE)
    {
      hash_val += (bint) keyval->values.join_style;
    }

  return hash_val;
}

static buint
btk_gc_value_hash (bpointer value)
{
  return (bulong) value;
}

static bboolean
btk_gc_key_equal (bpointer a,
		  bpointer b)
{
  BtkGCKey *akey;
  BtkGCKey *bkey;
  BdkGCValues *avalues;
  BdkGCValues *bvalues;

  akey = a;
  bkey = b;

  avalues = &akey->values;
  bvalues = &bkey->values;

  if (akey->mask != bkey->mask)
    return FALSE;

  if (akey->depth != bkey->depth)
    return FALSE;

  if (akey->colormap != bkey->colormap)
    return FALSE;

  if (akey->mask & BDK_GC_FOREGROUND)
    {
      if (avalues->foreground.pixel != bvalues->foreground.pixel)
	return FALSE;
    }
  if (akey->mask & BDK_GC_BACKGROUND)
    {
      if (avalues->background.pixel != bvalues->background.pixel)
	return FALSE;
    }
  if (akey->mask & BDK_GC_FONT)
    {
      if (!bdk_font_equal (avalues->font, bvalues->font))
	return FALSE;
    }
  if (akey->mask & BDK_GC_FUNCTION)
    {
      if (avalues->function != bvalues->function)
	return FALSE;
    }
  if (akey->mask & BDK_GC_FILL)
    {
      if (avalues->fill != bvalues->fill)
	return FALSE;
    }
  if (akey->mask & BDK_GC_TILE)
    {
      if (avalues->tile != bvalues->tile)
	return FALSE;
    }
  if (akey->mask & BDK_GC_STIPPLE)
    {
      if (avalues->stipple != bvalues->stipple)
	return FALSE;
    }
  if (akey->mask & BDK_GC_CLIP_MASK)
    {
      if (avalues->clip_mask != bvalues->clip_mask)
	return FALSE;
    }
  if (akey->mask & BDK_GC_SUBWINDOW)
    {
      if (avalues->subwindow_mode != bvalues->subwindow_mode)
	return FALSE;
    }
  if (akey->mask & BDK_GC_TS_X_ORIGIN)
    {
      if (avalues->ts_x_origin != bvalues->ts_x_origin)
	return FALSE;
    }
  if (akey->mask & BDK_GC_TS_Y_ORIGIN)
    {
      if (avalues->ts_y_origin != bvalues->ts_y_origin)
	return FALSE;
    }
  if (akey->mask & BDK_GC_CLIP_X_ORIGIN)
    {
      if (avalues->clip_x_origin != bvalues->clip_x_origin)
	return FALSE;
    }
  if (akey->mask & BDK_GC_CLIP_Y_ORIGIN)
    {
      if (avalues->clip_y_origin != bvalues->clip_y_origin)
	return FALSE;
    }
  if (akey->mask & BDK_GC_EXPOSURES)
    {
      if (avalues->graphics_exposures != bvalues->graphics_exposures)
	return FALSE;
    }
  if (akey->mask & BDK_GC_LINE_WIDTH)
    {
      if (avalues->line_width != bvalues->line_width)
	return FALSE;
    }
  if (akey->mask & BDK_GC_LINE_STYLE)
    {
      if (avalues->line_style != bvalues->line_style)
	return FALSE;
    }
  if (akey->mask & BDK_GC_CAP_STYLE)
    {
      if (avalues->cap_style != bvalues->cap_style)
	return FALSE;
    }
  if (akey->mask & BDK_GC_JOIN_STYLE)
    {
      if (avalues->join_style != bvalues->join_style)
	return FALSE;
    }

  return TRUE;
}

static buint
btk_gc_drawable_hash (BtkGCDrawable *d)
{
  return d->depth;
}

static bboolean
btk_gc_drawable_equal (BtkGCDrawable *a,
		       BtkGCDrawable *b)
{
  return (a->depth == b->depth);
}

#define __BTK_GC_C__
#include "btkaliasdef.c"
