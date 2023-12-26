/* BDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copyright (C) 1998-1999 Tor Lillqvist
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
 * file for a list of people on the BTK+ Team.
 */

/*
 * BTK+ DirectFB backend
 * Copyright (C) 2001-2002  convergence integrated media GmbH
 * Copyright (C) 2002-2004  convergence GmbH
 * Written by Denis Oliver Kropp <dok@convergence.de> and
 *            Sven Neumann <sven@convergence.de>
 */

#include "config.h"
#include "bdk.h"
#include "bdkwindowimpl.h"
#include "bdkwindow.h"

#include "bdkdirectfb.h"
#include "bdkprivate-directfb.h"
#include "bdkdisplay-directfb.h"

#include "bdkrebunnyion-generic.h"

#include "bdkinternals.h"
#include "bdkalias.h"
#include "bairo.h"
#include <assert.h>

#include <direct/debug.h>

#include <directfb_util.h>

D_DEBUG_DOMAIN (BDKDFB_Crossing,  "BDKDFB/Crossing",  "BDK DirectFB Crossing Events");
D_DEBUG_DOMAIN (BDKDFB_Updates,   "BDKDFB/Updates",   "BDK DirectFB Updates");
D_DEBUG_DOMAIN (BDKDFB_Paintable, "BDKDFB/Paintable", "BDK DirectFB Paintable");
D_DEBUG_DOMAIN (BDKDFB_Window,    "BDKDFB/Window",    "BDK DirectFB Window");


static BdkRebunnyion * bdk_window_impl_directfb_get_visible_rebunnyion (BdkDrawable *drawable);
static void        bdk_window_impl_directfb_set_colormap       (BdkDrawable *drawable,
                                                                BdkColormap *colormap);
static void bdk_window_impl_directfb_init       (BdkWindowImplDirectFB      *window);
static void bdk_window_impl_directfb_class_init (BdkWindowImplDirectFBClass *klass);
static void bdk_window_impl_directfb_finalize   (GObject                    *object);

static void bdk_window_impl_iface_init (BdkWindowImplIface *iface);
static void bdk_directfb_window_destroy (BdkWindow *window,
                                         gboolean   recursing,
                                         gboolean   foreign_destroy);

typedef struct
{
  BdkWindowChildChanged changed;
  BdkWindowChildGetPos  get_pos;
  gpointer              user_data;
} BdkWindowChildHandlerData;


static BdkWindow *bdk_directfb_window_containing_pointer = NULL;
static BdkWindow *bdk_directfb_focused_window            = NULL;
static gpointer   parent_class                           = NULL;
BdkWindow * _bdk_parent_root = NULL;

static void bdk_window_impl_directfb_paintable_init (BdkPaintableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BdkWindowImplDirectFB,
                         bdk_window_impl_directfb,
                         BDK_TYPE_DRAWABLE_IMPL_DIRECTFB,
                         G_IMPLEMENT_INTERFACE (BDK_TYPE_WINDOW_IMPL,
                                                bdk_window_impl_iface_init)
                         G_IMPLEMENT_INTERFACE (BDK_TYPE_PAINTABLE,
                                                bdk_window_impl_directfb_paintable_init));

GType
_bdk_window_impl_get_type (void)
{
  return bdk_window_impl_directfb_get_type ();
}

static void
bdk_window_impl_directfb_init (BdkWindowImplDirectFB *impl)
{
  impl->drawable.width  = 1;
  impl->drawable.height = 1;
  //cannot use bdk_cursor_new here since bdk_display_get_default
  //does not work yet.
  impl->cursor  = bdk_cursor_new_for_display (BDK_DISPLAY_OBJECT (_bdk_display),
                                              BDK_LEFT_PTR);
  impl->opacity = 255;
}

static void
bdk_window_impl_directfb_class_init (BdkWindowImplDirectFBClass *klass)
{
  GObjectClass     *object_class   = G_OBJECT_CLASS (klass);
  BdkDrawableClass *drawable_class = BDK_DRAWABLE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_window_impl_directfb_finalize;

  drawable_class->set_colormap = bdk_window_impl_directfb_set_colormap;

  /* Visible and clip rebunnyions are the same */

  drawable_class->get_clip_rebunnyion =
    bdk_window_impl_directfb_get_visible_rebunnyion;

  drawable_class->get_visible_rebunnyion =
    bdk_window_impl_directfb_get_visible_rebunnyion;
}

static void
g_free_2nd (gpointer a,
            gpointer b,
            gpointer data)
{
  g_free (b);
}

static void
bdk_window_impl_directfb_finalize (GObject *object)
{
  BdkWindowImplDirectFB *impl = BDK_WINDOW_IMPL_DIRECTFB (object);

  D_DEBUG_AT (BDKDFB_Window, "%s( %p ) <- %dx%d\n", G_STRFUNC,
              impl, impl->drawable.width, impl->drawable.height);

  if (BDK_WINDOW_IS_MAPPED (impl->drawable.wrapper))
    bdk_window_hide (impl->drawable.wrapper);

  if (impl->cursor)
    bdk_cursor_unref (impl->cursor);

  if (impl->properties)
    {
      g_hash_table_foreach (impl->properties, g_free_2nd, NULL);
      g_hash_table_destroy (impl->properties);
    }
  if (impl->window)
    {
      bdk_directfb_window_id_table_remove (impl->dfb_id);
      /* native window resource must be release before we can finalize !*/
      impl->window = NULL;
    }

  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}

static BdkRebunnyion *
bdk_window_impl_directfb_get_visible_rebunnyion (BdkDrawable *drawable)
{
  BdkDrawableImplDirectFB *priv  = BDK_DRAWABLE_IMPL_DIRECTFB (drawable);
  BdkRectangle             rect  = { 0, 0, 0, 0 };
  DFBRectangle             drect = { 0, 0, 0, 0 };

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, drawable);

  if (priv->surface)
    priv->surface->GetVisibleRectangle (priv->surface, &drect);
  rect.x      = drect.x;
  rect.y      = drect.y;
  rect.width  = drect.w;
  rect.height = drect.h;

  D_DEBUG_AT (BDKDFB_Window, "  -> returning %4d,%4d-%4dx%4d\n",
              drect.x, drect.y, drect.w, drect.h);

  return bdk_rebunnyion_rectangle (&rect);
}

static void
bdk_window_impl_directfb_set_colormap (BdkDrawable *drawable,
                                       BdkColormap *colormap)
{
  BDK_DRAWABLE_CLASS (parent_class)->set_colormap (drawable, colormap);

  if (colormap)
    {
      BdkDrawableImplDirectFB *priv = BDK_DRAWABLE_IMPL_DIRECTFB (drawable);

      if (priv->surface)
        {
          IDirectFBPalette *palette = bdk_directfb_colormap_get_palette (colormap);

          if (palette)
            priv->surface->SetPalette (priv->surface, palette);
        }
    }
}


static gboolean
create_directfb_window (BdkWindowImplDirectFB *impl,
                        DFBWindowDescription  *desc,
                        DFBWindowOptions       window_options)
{
  DFBResult        ret;
  IDirectFBWindow *window;

  D_DEBUG_AT (BDKDFB_Window, "%s( %4dx%4d, caps 0x%08x )\n", G_STRFUNC,
              desc->width, desc->height, desc->caps);

  ret = _bdk_display->layer->CreateWindow (_bdk_display->layer, desc, &window);

  if (ret != DFB_OK)
    {
      DirectFBError ("bdk_window_new: Layer->CreateWindow failed", ret);
      g_assert (0);
      return FALSE;
    }

  if ((desc->flags & DWDESC_CAPS) && (desc->caps & DWCAPS_INPUTONLY))
    {
      impl->drawable.surface = NULL;
    } else
    window->GetSurface (window, &impl->drawable.surface);

  if (window_options)
    {
      DFBWindowOptions options;
      window->GetOptions (window, &options);
      window->SetOptions (window,  options | window_options);
    }

  impl->window = window;

#ifndef BDK_DIRECTFB_NO_EXPERIMENTS
  //direct_log_printf (NULL, "Initializing (window %p, wimpl %p)\n", win, impl);

  dfb_updates_init (&impl->flips, impl->flip_rebunnyions,
                    G_N_ELEMENTS (impl->flip_rebunnyions));
#endif

  return TRUE;
}

void
_bdk_windowing_window_init (BdkScreen *screen)
{
  BdkWindowObject         *private;
  BdkDrawableImplDirectFB *draw_impl;
  DFBDisplayLayerConfig    dlc;

  g_assert (_bdk_parent_root == NULL);

  _bdk_display->layer->GetConfiguration (_bdk_display->layer, &dlc);

  _bdk_parent_root = g_object_new (BDK_TYPE_WINDOW, NULL);

  private = BDK_WINDOW_OBJECT (_bdk_parent_root);
  private->impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  private->impl_window = private;

  draw_impl = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);

  /* custom root window init */
  {
    DFBWindowDescription   desc;
    desc.flags = 0;
    /*XXX I must do this now its a bug  ALPHA ROOT*/

    desc.flags   = DWDESC_CAPS;
    desc.caps    = 0;
    desc.caps   |= DWCAPS_NODECORATION;
    desc.caps   |= DWCAPS_ALPHACHANNEL;
    desc.flags  |= (DWDESC_WIDTH | DWDESC_HEIGHT |
                    DWDESC_POSX  | DWDESC_POSY);
    desc.posx    = 0;
    desc.posy    = 0;
    desc.width   = dlc.width;
    desc.height  = dlc.height;

    create_directfb_window (BDK_WINDOW_IMPL_DIRECTFB (private->impl),
                            &desc, 0);

    g_assert (BDK_WINDOW_IMPL_DIRECTFB (private->impl)->window != NULL);
    g_assert (draw_impl->surface != NULL);
  }

  private->window_type = BDK_WINDOW_ROOT;
  private->viewable    = TRUE;
  private->x           = 0;
  private->y           = 0;
  private->abs_x       = 0;
  private->abs_y       = 0;
  private->width       = dlc.width;
  private->height      = dlc.height;

  //  impl->drawable.paint_rebunnyion = NULL;
  /* impl->window                    = NULL; */
  draw_impl->abs_x    = 0;
  draw_impl->abs_y    = 0;
  draw_impl->width    = dlc.width;
  draw_impl->height   = dlc.height;
  draw_impl->wrapper  = BDK_DRAWABLE (private);
  draw_impl->colormap = bdk_screen_get_system_colormap (screen);
  g_object_ref (draw_impl->colormap);

  draw_impl->surface->GetPixelFormat (draw_impl->surface,
                                      &draw_impl->format);
  private->depth = DFB_BITS_PER_PIXEL (draw_impl->format);

  _bdk_window_update_size (_bdk_parent_root);
}


BdkWindow *
bdk_directfb_window_new (BdkWindow              *parent,
                         BdkWindowAttr          *attributes,
                         gint                    attributes_mask,
                         DFBWindowCapabilities   window_caps,
                         DFBWindowOptions        window_options,
                         DFBSurfaceCapabilities  surface_caps)
{
  BdkWindow             *window;
  BdkWindowObject       *private;
  BdkWindowObject       *parent_private;
  BdkWindowImplDirectFB *impl;
  BdkWindowImplDirectFB *parent_impl;
  BdkVisual             *visual;
  DFBWindowDescription   desc;
  gint x, y;

  g_return_val_if_fail (attributes != NULL, NULL);

  D_DEBUG_AT( BDKDFB_Window, "%s( %p )\n", G_STRFUNC, parent );

  if (!parent || attributes->window_type != BDK_WINDOW_CHILD)
    parent = _bdk_parent_root;

  window = g_object_new (BDK_TYPE_WINDOW, NULL);
  private = BDK_WINDOW_OBJECT (window);
  private->impl = g_object_new (_bdk_window_impl_get_type (), NULL);

  parent_private = BDK_WINDOW_OBJECT (parent);
  parent_impl = BDK_WINDOW_IMPL_DIRECTFB (parent_private->impl);
  private->parent = parent_private;

  x = (attributes_mask & BDK_WA_X) ? attributes->x : 0;
  y = (attributes_mask & BDK_WA_Y) ? attributes->y : 0;

  bdk_window_set_events (window, attributes->event_mask | BDK_STRUCTURE_MASK);

  impl = BDK_WINDOW_IMPL_DIRECTFB (private->impl);
  impl->drawable.wrapper = BDK_DRAWABLE (window);

  private->x = x;
  private->y = y;

  impl->drawable.width  = MAX (1, attributes->width);
  impl->drawable.height = MAX (1, attributes->height);

  private->window_type = attributes->window_type;

  desc.flags = 0;

  if (attributes_mask & BDK_WA_VISUAL)
    visual = attributes->visual;
  else
    visual = bdk_drawable_get_visual (parent);

  switch (attributes->wclass)
    {
    case BDK_INPUT_OUTPUT:
      private->input_only = FALSE;

      desc.flags |= DWDESC_PIXELFORMAT;
      desc.pixelformat = ((BdkVisualDirectFB *) visual)->format;

      if (DFB_PIXELFORMAT_HAS_ALPHA (desc.pixelformat))
        {
          desc.flags |= DWDESC_CAPS;
          desc.caps = DWCAPS_ALPHACHANNEL;
        }
      break;

    case BDK_INPUT_ONLY:
      private->input_only = TRUE;
      desc.flags |= DWDESC_CAPS;
      desc.caps = DWCAPS_INPUTONLY;
      break;

    default:
      g_warning ("bdk_window_new: unsupported window class\n");
      _bdk_window_destroy (window, FALSE);
      return NULL;
    }

  switch (private->window_type)
    {
    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_DIALOG:
    case BDK_WINDOW_TEMP:
      desc.flags |= ( DWDESC_WIDTH | DWDESC_HEIGHT |
                      DWDESC_POSX  | DWDESC_POSY );
      desc.posx   = x;
      desc.posy   = y;
      desc.width  = impl->drawable.width;
      desc.height = impl->drawable.height;

#if 0
      if (window_caps)
        {
          if (! (desc.flags & DWDESC_CAPS))
            {
              desc.flags |= DWDESC_CAPS;
              desc.caps   = DWCAPS_NONE;
            }

          desc.caps |= window_caps;
        }

      if (surface_caps)
        {
          desc.flags |= DWDESC_SURFACE_CAPS;
          desc.surface_caps = surface_caps;
        }
#endif

      if (!create_directfb_window (impl, &desc, window_options))
        {
          g_assert(0);
          _bdk_window_destroy (window, FALSE);

          return NULL;
        }

      if (desc.caps != DWCAPS_INPUTONLY)
        {
          impl->window->SetOpacity(impl->window, 0x00 );
        }

      break;

    case BDK_WINDOW_CHILD:
      impl->window=NULL;

      if (!private->input_only && parent_impl->drawable.surface)
        {

          DFBRectangle rect =
          { x, y, impl->drawable.width, impl->drawable.height };
          parent_impl->drawable.surface->GetSubSurface (parent_impl->drawable.surface,
                                                        &rect,
                                                        &impl->drawable.surface);
        }

      break;

    default:
      g_warning ("bdk_window_new: unsupported window type: %d",
                 private->window_type);
      _bdk_window_destroy (window, FALSE);

      return NULL;
    }

  if (impl->drawable.surface)
    {
      BdkColormap *colormap;

      impl->drawable.surface->GetPixelFormat (impl->drawable.surface,
                                              &impl->drawable.format);

      private->depth = DFB_BITS_PER_PIXEL(impl->drawable.format);

      if ((attributes_mask & BDK_WA_COLORMAP) && attributes->colormap)
        {
          colormap = attributes->colormap;
        }
      else
        {
          if (bdk_visual_get_system () == visual)
            colormap = bdk_colormap_get_system ();
          else
            colormap =bdk_drawable_get_colormap (parent);
        }

      bdk_drawable_set_colormap (BDK_DRAWABLE (window), colormap);
    }
  else
    {
      impl->drawable.format = ((BdkVisualDirectFB *)visual)->format;
      private->depth = visual->depth;
    }

  bdk_window_set_cursor (window, ((attributes_mask & BDK_WA_CURSOR) ?
                                  (attributes->cursor) : NULL));

  if (parent_private)
    parent_private->children = g_list_prepend (parent_private->children,
                                               window);

  /* we hold a reference count on ourselves */
  g_object_ref (window);

  if (impl->window)
    {
      impl->window->GetID (impl->window, &impl->dfb_id);
      bdk_directfb_window_id_table_insert (impl->dfb_id, window);
      bdk_directfb_event_windows_add (window);
    }

  if (attributes_mask & BDK_WA_TYPE_HINT)
    bdk_window_set_type_hint (window, attributes->type_hint);

  return window;
}


void
_bdk_window_impl_new (BdkWindow     *window,
                      BdkWindow     *real_parent,
                      BdkScreen     *screen,
                      BdkVisual     *visual,
                      BdkEventMask   event_mask,
                      BdkWindowAttr *attributes,
                      gint           attributes_mask)
{
  BdkWindowObject       *private;
  BdkWindowObject       *parent_private;
  BdkWindowImplDirectFB *impl;
  BdkWindowImplDirectFB *parent_impl;
  DFBWindowDescription   desc;

  impl = g_object_new (_bdk_window_impl_get_type (), NULL);
  impl->drawable.wrapper = BDK_DRAWABLE (window);

  private = BDK_WINDOW_OBJECT (window);
  private->impl = (BdkDrawable *)impl;

  parent_private = BDK_WINDOW_OBJECT (real_parent);
  parent_impl = BDK_WINDOW_IMPL_DIRECTFB (parent_private->impl);

  impl->drawable.width  = MAX (1, attributes->width);
  impl->drawable.height = MAX (1, attributes->height);

  desc.flags = 0;

  switch (attributes->wclass)
    {
    case BDK_INPUT_OUTPUT:
      desc.flags       |= DWDESC_PIXELFORMAT;
      desc.pixelformat  = ((BdkVisualDirectFB *)visual)->format;

      if (DFB_PIXELFORMAT_HAS_ALPHA (desc.pixelformat))
        {
          desc.flags |= DWDESC_CAPS;
          desc.caps   = DWCAPS_ALPHACHANNEL;
        }

      break;

    case BDK_INPUT_ONLY:
      desc.flags          |= DWDESC_CAPS;
      desc.caps            = DWCAPS_INPUTONLY;
      break;

    default:
      g_warning ("_bdk_window_impl_new: unsupported window class\n");
      _bdk_window_destroy (window, FALSE);
      return;
    }

  switch (private->window_type)
    {
    case BDK_WINDOW_TOPLEVEL:
    case BDK_WINDOW_DIALOG:
    case BDK_WINDOW_TEMP:
      desc.flags |= (DWDESC_WIDTH | DWDESC_HEIGHT |
                     DWDESC_POSX  | DWDESC_POSY);
      desc.posx   = private->x;
      desc.posy   = private->y;
      desc.width  = impl->drawable.width;
      desc.height = impl->drawable.height;

      if (!create_directfb_window (impl, &desc, DWOP_NONE))
        {
          g_assert (0);
          _bdk_window_destroy (window, FALSE);

          return;
        }

      if (desc.caps != DWCAPS_INPUTONLY)
        {
          impl->window->SetOpacity (impl->window, 0x00);
        }

      break;

    case BDK_WINDOW_CHILD:
      impl->window = NULL;

      if (!private->input_only && parent_impl->drawable.surface)
        {
          DFBRectangle rect = { private->x,
                                private->y,
                                impl->drawable.width,
                                impl->drawable.height };
          parent_impl->drawable.surface->GetSubSurface (parent_impl->drawable.surface,
                                                        &rect,
                                                        &impl->drawable.surface);
        }

      break;

    default:
      g_warning ("_bdk_window_impl_new: unsupported window type: %d",
                 private->window_type);
      _bdk_window_destroy (window, FALSE);

      return;
    }

  if (impl->drawable.surface)
    {
      BdkColormap *colormap;

      impl->drawable.surface->GetPixelFormat (impl->drawable.surface,
                                              &impl->drawable.format);

      if ((attributes_mask & BDK_WA_COLORMAP) && attributes->colormap)
        {
          colormap = attributes->colormap;
        }
      else
        {
          if (bdk_visual_get_system () == visual)
            colormap = bdk_colormap_get_system ();
          else
            colormap = bdk_colormap_new (visual, FALSE);
        }

      bdk_drawable_set_colormap (BDK_DRAWABLE (window), colormap);
    }
  else
    {
      impl->drawable.format = ((BdkVisualDirectFB *)visual)->format;
    }

  bdk_window_set_cursor (window,
                         ((attributes_mask & BDK_WA_CURSOR) ?
                          (attributes->cursor) : NULL));

  /* we hold a reference count on ourself */
  g_object_ref (window);

  if (impl->window)
    {
      impl->window->GetID (impl->window, &impl->dfb_id);
      bdk_directfb_window_id_table_insert (impl->dfb_id, window);
      bdk_directfb_event_windows_add (window);
    }

  if (attributes_mask & BDK_WA_TYPE_HINT)
    bdk_window_set_type_hint (window, attributes->type_hint);
}

void
_bdk_windowing_window_destroy_foreign (BdkWindow *window)
{
  /* It's somebody else's window, but in our hierarchy,
   * so reparent it to the root window, and then send
   * it a delete event, as if we were a WM
   */
  bdk_directfb_window_destroy (window, TRUE, TRUE);
}

static void
bdk_directfb_window_destroy (BdkWindow *window,
                             gboolean   recursing,
                             gboolean   foreign_destroy)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, %srecursing, %sforeign )\n", G_STRFUNC, window,
              recursing ? "" : "not ", foreign_destroy ? "" : "no ");

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  _bdk_selection_window_destroyed (window);
  bdk_directfb_event_windows_remove (window);

  if (window == _bdk_directfb_pointer_grab_window)
    bdk_pointer_ungrab (BDK_CURRENT_TIME);
  if (window == _bdk_directfb_keyboard_grab_window)
    bdk_keyboard_ungrab (BDK_CURRENT_TIME);

  if (window == bdk_directfb_focused_window)
    bdk_directfb_change_focus (NULL);

  if (impl->drawable.surface) {
    BdkDrawableImplDirectFB *dimpl = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);
    if (dimpl->bairo_surface)
      bairo_surface_destroy (dimpl->bairo_surface);
    impl->drawable.surface->Release (impl->drawable.surface);
    impl->drawable.surface = NULL;
  }

  if (!recursing && !foreign_destroy && impl->window ) {
    impl->window->SetOpacity (impl->window, 0);
    impl->window->Close (impl->window);
    impl->window->Release (impl->window);
    impl->window = NULL;
  }
}

/* This function is called when the window is really gone.
 */
void
bdk_window_destroy_notify (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  if (!BDK_WINDOW_DESTROYED (window))
    {
      if (BDK_WINDOW_TYPE(window) != BDK_WINDOW_FOREIGN)
	g_warning ("BdkWindow %p unexpectedly destroyed", window);

      _bdk_window_destroy (window, TRUE);
    }
  g_object_unref (window);
}

/* Focus follows pointer */
BdkWindow *
bdk_directfb_window_find_toplevel (BdkWindow *window)
{
  while (window && window != _bdk_parent_root)
    {
      BdkWindow *parent = (BdkWindow *) (BDK_WINDOW_OBJECT (window))->parent;

      if ((parent == _bdk_parent_root) && BDK_WINDOW_IS_MAPPED (window))
        return window;

      window = parent;
    }

  return _bdk_parent_root;
}

BdkWindow *
bdk_directfb_window_find_focus (void)
{
  if (_bdk_directfb_keyboard_grab_window)
    return _bdk_directfb_keyboard_grab_window;

  if (!bdk_directfb_focused_window)
    bdk_directfb_focused_window = g_object_ref (_bdk_parent_root);

  return bdk_directfb_focused_window;
}

void
bdk_directfb_change_focus (BdkWindow *new_focus_window)
{
  BdkEventFocus *event;
  BdkWindow     *old_win;
  BdkWindow     *new_win;
  BdkWindow     *event_win;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, new_focus_window);

  /* No focus changes while the pointer is grabbed */
  if (_bdk_directfb_pointer_grab_window)
    return;

  old_win = bdk_directfb_focused_window;
  new_win = bdk_directfb_window_find_toplevel (new_focus_window);

  if (old_win == new_win)
    return;

  if (old_win)
    {
      event_win = bdk_directfb_keyboard_event_window (old_win,
                                                      BDK_FOCUS_CHANGE);
      if (event_win)
        {
          event = (BdkEventFocus *) bdk_directfb_event_make (event_win,
                                                             BDK_FOCUS_CHANGE);
          event->in = FALSE;
        }
    }

  event_win = bdk_directfb_keyboard_event_window (new_win,
                                                  BDK_FOCUS_CHANGE);
  if (event_win)
    {
      event = (BdkEventFocus *) bdk_directfb_event_make (event_win,
                                                         BDK_FOCUS_CHANGE);
      event->in = TRUE;
    }

  if (bdk_directfb_focused_window)
    g_object_unref (bdk_directfb_focused_window);
  bdk_directfb_focused_window = g_object_ref (new_win);
}

void
bdk_window_set_accept_focus (BdkWindow *window,
                             gboolean accept_focus)
{
  BdkWindowObject *private;
  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  private = (BdkWindowObject *)window;

  accept_focus = accept_focus != FALSE;

  if (private->accept_focus != accept_focus)
    private->accept_focus = accept_focus;

}

void
bdk_window_set_focus_on_map (BdkWindow *window,
                             gboolean focus_on_map)
{
  BdkWindowObject *private;
  g_return_if_fail (window != NULL);
  g_return_if_fail (BDK_IS_WINDOW (window));

  private = (BdkWindowObject *)window;

  focus_on_map = focus_on_map != FALSE;

  if (private->focus_on_map != focus_on_map)
    private->focus_on_map = focus_on_map;
}

static gboolean
bdk_directfb_window_raise (BdkWindow *window)
{
  BdkWindowObject *parent;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  parent = BDK_WINDOW_OBJECT (window)->parent;

  if (parent->children->data == window)
    return FALSE;

  parent->children = g_list_remove (parent->children, window);
  parent->children = g_list_prepend (parent->children, window);

  return TRUE;
}

static void
bdk_directfb_window_lower (BdkWindow *window)
{
  BdkWindowObject *parent;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  parent = BDK_WINDOW_OBJECT (window)->parent;

  parent->children = g_list_remove (parent->children, window);
  parent->children = g_list_append (parent->children, window);
}

static gboolean
all_parents_shown (BdkWindowObject *private)
{
  while (BDK_WINDOW_IS_MAPPED (private))
    {
      if (private->parent)
        private = BDK_WINDOW_OBJECT (private)->parent;
      else
        return TRUE;
    }

  return FALSE;
}

static void
send_map_events (BdkWindowObject *private)
{
  GList     *list;
  BdkWindow *event_win;

  if (!BDK_WINDOW_IS_MAPPED (private))
    return;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, private);

  event_win = bdk_directfb_other_event_window ((BdkWindow *) private, BDK_MAP);
  if (event_win)
    bdk_directfb_event_make (event_win, BDK_MAP);

  for (list = private->children; list; list = list->next)
    send_map_events (list->data);
}

static BdkWindow *
bdk_directfb_find_common_ancestor (BdkWindow *win1,
                                   BdkWindow *win2)
{
  BdkWindowObject *a;
  BdkWindowObject *b;

  for (a = BDK_WINDOW_OBJECT (win1); a; a = a->parent)
    for (b = BDK_WINDOW_OBJECT (win2); b; b = b->parent)
      {
        if (a == b)
          return BDK_WINDOW (a);
      }

  return NULL;
}

void
bdk_directfb_window_send_crossing_events (BdkWindow       *src,
                                          BdkWindow       *dest,
                                          BdkCrossingMode  mode)
{
  BdkWindow       *c;
  BdkWindow       *win, *last, *next;
  BdkEvent        *event;
  gint             x, y, x_int, y_int;
  BdkModifierType  modifiers;
  GSList          *path, *list;
  gboolean         non_linear;
  BdkWindow       *a;
  BdkWindow       *b;
  BdkWindow       *event_win;

  D_DEBUG_AT (BDKDFB_Crossing, "%s( %p -> %p, %d )\n", G_STRFUNC, src, dest, mode);

  /* Do a possible cursor change before checking if we need to
     generate crossing events so cursor changes due to pointer
     grabs work correctly. */
  {
    static BdkCursorDirectFB *last_cursor = NULL;

    BdkWindowObject       *private = BDK_WINDOW_OBJECT (dest);
    BdkWindowImplDirectFB *impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);
    BdkCursorDirectFB     *cursor;

    if (_bdk_directfb_pointer_grab_cursor)
      cursor = (BdkCursorDirectFB*) _bdk_directfb_pointer_grab_cursor;
    else
      cursor = (BdkCursorDirectFB*) impl->cursor;

    if (cursor != last_cursor)
      {
        win     = bdk_directfb_window_find_toplevel (dest);
        private = BDK_WINDOW_OBJECT (win);
        impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

        if (impl->window)
          impl->window->SetCursorShape (impl->window,
                                        cursor->shape,
                                        cursor->hot_x, cursor->hot_y);
        last_cursor = cursor;
      }
  }

  if (dest == bdk_directfb_window_containing_pointer) {
    D_DEBUG_AT (BDKDFB_Crossing, "  -> already containing the pointer\n");
    return;
  }

  if (bdk_directfb_window_containing_pointer == NULL)
    bdk_directfb_window_containing_pointer = g_object_ref (_bdk_parent_root);

  if (src)
    a = src;
  else
    a = bdk_directfb_window_containing_pointer;

  b = dest;

  if (a == b) {
    D_DEBUG_AT (BDKDFB_Crossing, "  -> src == dest\n");
    return;
  }

  /* bdk_directfb_window_containing_pointer might have been destroyed.
   * The refcount we hold on it should keep it, but it's parents
   * might have died.
   */
  if (BDK_WINDOW_DESTROYED (a)) {
    D_DEBUG_AT (BDKDFB_Crossing, "  -> src is destroyed!\n");
    a = _bdk_parent_root;
  }

  bdk_directfb_mouse_get_info (&x, &y, &modifiers);

  c = bdk_directfb_find_common_ancestor (a, b);

  D_DEBUG_AT (BDKDFB_Crossing, "  -> common ancestor %p\n", c);

  non_linear = (c != a) && (c != b);

  D_DEBUG_AT (BDKDFB_Crossing, "  -> non_linear: %s\n", non_linear ? "YES" : "NO");

  event_win = bdk_directfb_pointer_event_window (a, BDK_LEAVE_NOTIFY);
  if (event_win)
    {
      D_DEBUG_AT (BDKDFB_Crossing, "  -> sending LEAVE to src\n");

      event = bdk_directfb_event_make (event_win, BDK_LEAVE_NOTIFY);
      event->crossing.subwindow = NULL;

      bdk_window_get_origin (a, &x_int, &y_int);

      event->crossing.x      = x - x_int;
      event->crossing.y      = y - y_int;
      event->crossing.x_root = x;
      event->crossing.y_root = y;
      event->crossing.mode   = mode;

      if (non_linear)
        event->crossing.detail = BDK_NOTIFY_NONLINEAR;
      else if (c == a)
        event->crossing.detail = BDK_NOTIFY_INFERIOR;
      else
        event->crossing.detail = BDK_NOTIFY_ANCESTOR;

      event->crossing.focus = FALSE;
      event->crossing.state = modifiers;

      D_DEBUG_AT (BDKDFB_Crossing,
                  "  => LEAVE (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                  event_win, a,
                  event->crossing.x, event->crossing.y,
                  event->crossing.x_root, event->crossing.y_root,
                  event->crossing.mode, event->crossing.detail);
    }

  /* Traverse up from a to (excluding) c */
  if (c != a)
    {
      last = a;
      win = BDK_WINDOW (BDK_WINDOW_OBJECT (a)->parent);
      while (win != c)
        {
          event_win =
            bdk_directfb_pointer_event_window (win, BDK_LEAVE_NOTIFY);

          if (event_win)
            {
              event = bdk_directfb_event_make (event_win, BDK_LEAVE_NOTIFY);

              event->crossing.subwindow = g_object_ref (last);

              bdk_window_get_origin (win, &x_int, &y_int);

              event->crossing.x      = x - x_int;
              event->crossing.y      = y - y_int;
              event->crossing.x_root = x;
              event->crossing.y_root = y;
              event->crossing.mode   = mode;

              if (non_linear)
                event->crossing.detail = BDK_NOTIFY_NONLINEAR_VIRTUAL;
              else
                event->crossing.detail = BDK_NOTIFY_VIRTUAL;

              event->crossing.focus = FALSE;
              event->crossing.state = modifiers;

              D_DEBUG_AT (BDKDFB_Crossing,
                          "  -> LEAVE (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                          event_win, win,
                          event->crossing.x, event->crossing.y,
                          event->crossing.x_root, event->crossing.y_root,
                          event->crossing.mode, event->crossing.detail);
            }

          last = win;
          win = BDK_WINDOW (BDK_WINDOW_OBJECT (win)->parent);
        }
    }

  /* Traverse down from c to b */
  if (c != b)
    {
      path = NULL;
      win = BDK_WINDOW (BDK_WINDOW_OBJECT (b)->parent);
      while (win != c)
        {
          path = g_slist_prepend (path, win);
          win = BDK_WINDOW (BDK_WINDOW_OBJECT (win)->parent);
        }

      list = path;
      while (list)
        {
          win = BDK_WINDOW (list->data);
          list = g_slist_next (list);

          if (list)
            next = BDK_WINDOW (list->data);
          else
            next = b;

          event_win =
            bdk_directfb_pointer_event_window (win, BDK_ENTER_NOTIFY);

          if (event_win)
            {
              event = bdk_directfb_event_make (event_win, BDK_ENTER_NOTIFY);

              event->crossing.subwindow = g_object_ref (next);

              bdk_window_get_origin (win, &x_int, &y_int);

              event->crossing.x      = x - x_int;
              event->crossing.y      = y - y_int;
              event->crossing.x_root = x;
              event->crossing.y_root = y;
              event->crossing.mode   = mode;

              if (non_linear)
                event->crossing.detail = BDK_NOTIFY_NONLINEAR_VIRTUAL;
              else
                event->crossing.detail = BDK_NOTIFY_VIRTUAL;

              event->crossing.focus = FALSE;
              event->crossing.state = modifiers;

              D_DEBUG_AT (BDKDFB_Crossing,
                          "  -> ENTER (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                          event_win, win,
                          event->crossing.x, event->crossing.y,
                          event->crossing.x_root, event->crossing.y_root,
                          event->crossing.mode, event->crossing.detail);
            }
        }

      g_slist_free (path);
    }

  event_win = bdk_directfb_pointer_event_window (b, BDK_ENTER_NOTIFY);
  if (event_win)
    {
      event = bdk_directfb_event_make (event_win, BDK_ENTER_NOTIFY);

      event->crossing.subwindow = NULL;

      bdk_window_get_origin (b, &x_int, &y_int);

      event->crossing.x      = x - x_int;
      event->crossing.y      = y - y_int;
      event->crossing.x_root = x;
      event->crossing.y_root = y;
      event->crossing.mode   = mode;

      if (non_linear)
        event->crossing.detail = BDK_NOTIFY_NONLINEAR;
      else if (c==a)
        event->crossing.detail = BDK_NOTIFY_ANCESTOR;
      else
        event->crossing.detail = BDK_NOTIFY_INFERIOR;

      event->crossing.focus = FALSE;
      event->crossing.state = modifiers;

      D_DEBUG_AT (BDKDFB_Crossing,
                  "  => ENTER (%p/%p) at %4f,%4f (%4f,%4f) mode %d, detail %d\n",
                  event_win, b,
                  event->crossing.x, event->crossing.y,
                  event->crossing.x_root, event->crossing.y_root,
                  event->crossing.mode, event->crossing.detail);
    }

  if (mode != BDK_CROSSING_GRAB)
    {
      //this seems to cause focus to change as the pointer moves yuck
      //bdk_directfb_change_focus (b);
      if (b != bdk_directfb_window_containing_pointer)
        {
          g_object_unref (bdk_directfb_window_containing_pointer);
          bdk_directfb_window_containing_pointer = g_object_ref (b);
        }
    }
}

static void
show_window_internal (BdkWindow *window,
                      gboolean   raise)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;
  BdkWindow             *mousewin;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, %sraise )\n", G_STRFUNC,
              window, raise ? "" : "no ");

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  if (!private->destroyed && !BDK_WINDOW_IS_MAPPED (private))
    {
      if (raise)
        bdk_window_raise (window);

      if (all_parents_shown (BDK_WINDOW_OBJECT (private)->parent))
        {
          send_map_events (private);

          mousewin = bdk_window_at_pointer (NULL, NULL);
          bdk_directfb_window_send_crossing_events (NULL, mousewin,
                                                    BDK_CROSSING_NORMAL);

          if (private->input_only)
            return;

          bdk_window_invalidate_rect (window, NULL, TRUE);
        }
    }

  if (impl->window)
    {
      if (bdk_directfb_apply_focus_opacity)
	impl->window->SetOpacity (impl->window,
				  (impl->opacity >> 1) + (impl->opacity >> 2));
      else
	impl->window->SetOpacity (impl->window, impl->opacity);
      /* if its the first window focus it */
    }
}

static void
bdk_directfb_window_show (BdkWindow *window,
                          gboolean   raise)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  show_window_internal (window, raise);
}

static void
bdk_directfb_window_hide (BdkWindow *window)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;
  BdkWindow             *mousewin;
  BdkWindow             *event_win;

  g_return_if_fail (BDK_IS_WINDOW (window));

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  if (impl->window)
    impl->window->SetOpacity (impl->window, 0);

  if (!private->destroyed && BDK_WINDOW_IS_MAPPED (private))
    {
      BdkEvent *event;

      if (!private->input_only && private->parent)
        {
          bdk_window_clear_area (BDK_WINDOW (private->parent),
                                 private->x,
                                 private->y,
                                 impl->drawable.width,
                                 impl->drawable.height);
        }

      event_win = bdk_directfb_other_event_window (window, BDK_UNMAP);
      if (event_win)
        event = bdk_directfb_event_make (event_win, BDK_UNMAP);

      mousewin = bdk_window_at_pointer (NULL, NULL);
      bdk_directfb_window_send_crossing_events (NULL,
                                                mousewin,
                                                BDK_CROSSING_NORMAL);

      if (window == _bdk_directfb_pointer_grab_window)
        bdk_pointer_ungrab (BDK_CURRENT_TIME);
      if (window == _bdk_directfb_keyboard_grab_window)
        bdk_keyboard_ungrab (BDK_CURRENT_TIME);
    }
}

static void
bdk_directfb_window_withdraw (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  /* for now this should be enough */
  bdk_window_hide (window);
}

void
_bdk_directfb_move_resize_child (BdkWindow *window,
                                 gint       x,
                                 gint       y,
                                 gint       width,
                                 gint       height)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;
  BdkWindowImplDirectFB *parent_impl;
  GList                 *list;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  impl->drawable.width  = width;
  impl->drawable.height = height;

  if (!private->input_only)
    {
      if (impl->drawable.surface)
        {
          if (impl->drawable.bairo_surface)
            bairo_surface_destroy (impl->drawable.bairo_surface);

          impl->drawable.surface->Release (impl->drawable.surface);
          impl->drawable.surface = NULL;
        }

      parent_impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (private->parent)->impl);

      if (parent_impl->drawable.surface)
        {
          DFBRectangle rect = { x, y, width, height };

          parent_impl->drawable.surface->GetSubSurface (parent_impl->drawable.surface,
                                                        &rect,
                                                        &impl->drawable.surface);
        }
    }
}

static  void
bdk_directfb_window_move (BdkWindow *window,
                          gint       x,
                          gint       y)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  if (impl->window)
    {
      impl->window->MoveTo (impl->window, x, y);
    }
  else
    {
      gint         width  = impl->drawable.width;
      gint         height = impl->drawable.height;
      BdkRectangle old    = { private->x, private->y,width,height };

      _bdk_directfb_move_resize_child (window, x, y, width, height);

      if (BDK_WINDOW_IS_MAPPED (private))
        {
          BdkWindow    *mousewin;
          BdkRectangle  new = { x, y, width, height };

          bdk_rectangle_union (&new, &old, &new);
          bdk_window_invalidate_rect (BDK_WINDOW (private->parent), &new,TRUE);

          /* The window the pointer is in might have changed */
          mousewin = bdk_window_at_pointer (NULL, NULL);
          bdk_directfb_window_send_crossing_events (NULL, mousewin,
                                                    BDK_CROSSING_NORMAL);
        }
    }
}

static void
bdk_directfb_window_move_resize (BdkWindow *window,
                                 gboolean   with_move,
                                 gint       x,
                                 gint       y,
                                 gint       width,
                                 gint       height)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = BDK_WINDOW_OBJECT (window);
  impl = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  if (with_move && (width < 0 && height < 0))
    {
      bdk_directfb_window_move (window, x, y);
      return;
    }

  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;

  if (private->destroyed ||
      (private->x == x  &&  private->y == y  &&
       impl->drawable.width == width  &&  impl->drawable.height == height))
    return;

  if (private->parent && (private->parent->window_type != BDK_WINDOW_CHILD))
    {
      BdkWindowChildHandlerData *data;

      data = g_object_get_data (G_OBJECT (private->parent),
                                "bdk-window-child-handler");

      if (data &&
          (*data->changed) (window, x, y, width, height, data->user_data))
        return;
    }

  if (impl->drawable.width == width  &&  impl->drawable.height == height)
    {
      if (with_move)
        bdk_directfb_window_move (window, x, y);
    }
  else if (impl->window)
    {
      if (with_move) {
        impl->window->MoveTo (impl->window, x, y);
      }
      impl->drawable.width = width;
      impl->drawable.height = height;

      impl->window->Resize (impl->window, width, height);
    }
  else
    {
      BdkRectangle old = { private->x, private->y,
                           impl->drawable.width, impl->drawable.height };
      BdkRectangle new = { x, y, width, height };

      if (! with_move)
        {
          new.x = private->x;
          new.y = private->y;
        }

      _bdk_directfb_move_resize_child (window,
                                       new.x, new.y, new.width, new.height);

      if (BDK_WINDOW_IS_MAPPED (private))
        {
          BdkWindow *mousewin;

          bdk_rectangle_union (&new, &old, &new);
          bdk_window_invalidate_rect (BDK_WINDOW (private->parent), &new,TRUE);

          /* The window the pointer is in might have changed */
          mousewin = bdk_window_at_pointer (NULL, NULL);
          bdk_directfb_window_send_crossing_events (NULL, mousewin,
                                                    BDK_CROSSING_NORMAL);
        }
    }
}

static gboolean
bdk_directfb_window_reparent (BdkWindow *window,
                              BdkWindow *new_parent,
                              gint       x,
                              gint       y)
{
  BdkWindowObject *window_private;
  BdkWindowObject *parent_private;
  BdkWindowObject *old_parent_private;
  BdkWindowImplDirectFB *impl;
  BdkWindowImplDirectFB *parent_impl;
  BdkVisual             *visual;

  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  if (BDK_WINDOW_DESTROYED (window))
    return FALSE;

  if (!new_parent)
    new_parent = _bdk_parent_root;

  window_private     = (BdkWindowObject *) window;
  old_parent_private = (BdkWindowObject *) window_private->parent;
  parent_private     = (BdkWindowObject *) new_parent;
  parent_impl        = BDK_WINDOW_IMPL_DIRECTFB (parent_private->impl);
  visual             = bdk_drawable_get_visual (window);

  /* already parented */
  if (window_private->parent == (BdkWindowObject *) new_parent)
    return FALSE;

  window_private->parent = (BdkWindowObject *) new_parent;

  impl = BDK_WINDOW_IMPL_DIRECTFB (window_private->impl);

  if (impl->drawable.surface) {
    impl->drawable.surface->Release (impl->drawable.surface);
    impl->drawable.surface = NULL;
  }

  if (impl->window != NULL) {
    bdk_directfb_window_id_table_remove (impl->dfb_id);
    impl->window->SetOpacity (impl->window, 0);
    impl->window->Close (impl->window);
    impl->window->Release (impl->window);
    impl->window = NULL;
  }

  //create window were a child of the root now
  if (window_private->parent == (BdkWindowObject *)_bdk_parent_root)  {
    DFBWindowDescription  desc;
    DFBWindowOptions  window_options = DWOP_NONE;
    desc.flags = DWDESC_CAPS;
    if (window_private->input_only) {
      desc.caps = DWCAPS_INPUTONLY;
    } else {
      desc.flags |= DWDESC_PIXELFORMAT;
      desc.pixelformat = ((BdkVisualDirectFB *) visual)->format;
      if (DFB_PIXELFORMAT_HAS_ALPHA (desc.pixelformat)) {
        desc.flags |= DWDESC_CAPS;
        desc.caps = DWCAPS_ALPHACHANNEL;
      }
    }
    if (window_private->window_type ==  BDK_WINDOW_CHILD)
      window_private->window_type = BDK_WINDOW_TOPLEVEL;
    desc.flags |= (DWDESC_WIDTH | DWDESC_HEIGHT |
                   DWDESC_POSX  | DWDESC_POSY);
    desc.posx   = x;
    desc.posy   = y;
    desc.width  = impl->drawable.width;
    desc.height = impl->drawable.height;
    if (!create_directfb_window (impl, &desc, window_options))
      {
        g_assert (0);
        _bdk_window_destroy (window, FALSE);
        return FALSE;
      }
    /* we hold a reference count on ourselves */
    g_object_ref (window);
    impl->window->GetID (impl->window, &impl->dfb_id);
    bdk_directfb_window_id_table_insert (impl->dfb_id, window);
    bdk_directfb_event_windows_add (window);
  } else {
    DFBRectangle rect = { x, y,
                          impl->drawable.width,
                          impl->drawable.height };
    impl->window = NULL;
    parent_impl->drawable.surface->GetSubSurface (parent_impl->drawable.surface,
                                                  &rect,
                                                  &impl->drawable.surface);
  }

  return TRUE;
}

static void
bdk_window_directfb_raise (BdkWindow *window)
{
  BdkWindowImplDirectFB *impl;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  if (impl->window)
    {
      DFBResult ret;

      ret = impl->window->RaiseToTop (impl->window);
      if (ret)
        DirectFBError ("bdkwindow-directfb.c: RaiseToTop", ret);
      else
        bdk_directfb_window_raise (window);
    }
  else
    {
      if (bdk_directfb_window_raise (window))
        bdk_window_invalidate_rect (window, NULL, TRUE);
    }
}

static void
bdk_window_directfb_lower (BdkWindow *window)
{
  BdkWindowImplDirectFB *impl;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  if (impl->window)
    {
      DFBResult ret;

      ret = impl->window->LowerToBottom (impl->window);
      if (ret)
        DirectFBError ("bdkwindow-directfb.c: LowerToBottom", ret);
      else
        bdk_directfb_window_lower (window);
    }
  else
    {
      bdk_directfb_window_lower (window);
      bdk_window_invalidate_rect (window, NULL, TRUE);
    }
}

void
bdk_window_set_hints (BdkWindow *window,
                      gint       x,
                      gint       y,
                      gint       min_width,
                      gint       min_height,
                      gint       max_width,
                      gint       max_height,
                      gint       flags)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  D_DEBUG_AT( BDKDFB_Window, "%s( %p, %3d,%3d, min %4dx%4d, max %4dx%4d, flags 0x%08x )\n", G_STRFUNC,
              window, x,y, min_width, min_height, max_width, max_height, flags );
  /* N/A */
}

void
bdk_window_set_geometry_hints (BdkWindow         *window,
                               const BdkGeometry *geometry,
                               BdkWindowHints     geom_mask)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_set_title (BdkWindow   *window,
                      const gchar *title)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, '%s' )\n", G_STRFUNC, window, title);
  /* N/A */
  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, window);
}

void
bdk_window_set_role (BdkWindow   *window,
                     const gchar *role)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

/**
 * bdk_window_set_startup_id:
 * @window: a toplevel #BdkWindow
 * @startup_id: a string with startup-notification identifier
 *
 * When using BTK+, typically you should use btk_window_set_startup_id()
 * instead of this low-level function.
 *
 * Since: 2.12
 *
 **/
void
bdk_window_set_startup_id (BdkWindow   *window,
                           const gchar *startup_id)
{
}

void
bdk_window_set_transient_for (BdkWindow *window,
                              BdkWindow *parent)
{
  BdkWindowObject *private;
  BdkWindowObject *root;
  gint i;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (BDK_IS_WINDOW (parent));

  private = BDK_WINDOW_OBJECT (window);
  root    = BDK_WINDOW_OBJECT (_bdk_parent_root);

  g_return_if_fail (BDK_WINDOW (private->parent) == _bdk_parent_root);
  g_return_if_fail (BDK_WINDOW (BDK_WINDOW_OBJECT (parent)->parent) == _bdk_parent_root);

  root->children = g_list_remove (root->children, window);

  i = g_list_index (root->children, parent);
  if (i < 0)
    root->children = g_list_prepend (root->children, window);
  else
    root->children = g_list_insert (root->children, window, i);
}

static void
bdk_directfb_window_set_background (BdkWindow      *window,
                                    const BdkColor *color)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (color != NULL);

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, %d,%d,%d )\n", G_STRFUNC,
              window, color->red, color->green, color->blue);
}

static void
bdk_directfb_window_set_back_pixmap (BdkWindow *window,
                                     BdkPixmap *pixmap)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, %p )\n", G_STRFUNC, window, pixmap);
}

static void
bdk_directfb_window_set_cursor (BdkWindow *window,
                                BdkCursor *cursor)
{
  BdkWindowImplDirectFB *impl;
  BdkCursor             *old_cursor;

  g_return_if_fail (BDK_IS_WINDOW (window));

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);
  old_cursor = impl->cursor;

  impl->cursor = (cursor ?
                  bdk_cursor_ref (cursor) : bdk_cursor_new (BDK_LEFT_PTR));

  if (bdk_window_at_pointer (NULL, NULL) == window)
    {
      /* This is a bit evil but we want to keep all cursor changes in
         one place, so let bdk_directfb_window_send_crossing_events
         do the work for us. */

      bdk_directfb_window_send_crossing_events (window, window,
                                                BDK_CROSSING_NORMAL);
    }
  else if (impl->window)
    {
      BdkCursorDirectFB *dfb_cursor = (BdkCursorDirectFB *) impl->cursor;

      /* this branch takes care of setting the cursor for unmapped windows */

      impl->window->SetCursorShape (impl->window,
                                    dfb_cursor->shape,
                                    dfb_cursor->hot_x, dfb_cursor->hot_y);
    }

  if (old_cursor)
    bdk_cursor_unref (old_cursor);
}

static void
bdk_directfb_window_get_geometry (BdkWindow *window,
                                  gint      *x,
                                  gint      *y,
                                  gint      *width,
                                  gint      *height,
                                  gint      *depth)
{
  BdkWindowObject         *private;
  BdkDrawableImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);

  if (!BDK_WINDOW_DESTROYED (window))
    {
      if (x)
	*x = private->x;

      if (y)
	*y = private->y;

      if (width)
	*width = impl->width;

      if (height)
	*height = impl->height;

      if (depth)
	*depth = DFB_BITS_PER_PIXEL (impl->format);
    }
}

static gboolean
bdk_directfb_window_get_deskrelative_origin (BdkWindow *window,
                                             gint      *x,
                                             gint      *y)
{
  return bdk_window_get_origin (window, x, y);
}

void
bdk_window_get_root_origin (BdkWindow *window,
                            gint      *x,
                            gint      *y)
{
  BdkWindowObject *rover;

  g_return_if_fail (BDK_IS_WINDOW (window));

  rover = (BdkWindowObject*) window;
  if (x)
    *x = 0;
  if (y)
    *y = 0;

  if (BDK_WINDOW_DESTROYED (window))
    return;

  while (rover->parent && ((BdkWindowObject*) rover->parent)->parent)
    rover = (BdkWindowObject *) rover->parent;
  if (rover->destroyed)
    return;

  if (x)
    *x = rover->x;
  if (y)
    *y = rover->y;
}

BdkWindow *
bdk_directfb_window_get_pointer_helper (BdkWindow       *window,
                                        gint            *x,
                                        gint            *y,
                                        BdkModifierType *mask)
{
  BdkWindow               *retval = NULL;
  gint                     rx, ry, wx, wy;
  BdkDrawableImplDirectFB *impl;

  g_return_val_if_fail (window == NULL || BDK_IS_WINDOW (window), NULL);

  if (!window)
    window = _bdk_parent_root;

  bdk_directfb_mouse_get_info (&rx, &ry, mask);

  wx = rx;
  wy = ry;
  retval = bdk_directfb_child_at (_bdk_parent_root, &wx, &wy);

  impl = BDK_DRAWABLE_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  if (x)
    *x = rx - impl->abs_x;
  if (y)
    *y = ry - impl->abs_y;

  return retval;
}

static gboolean
bdk_directfb_window_get_pointer (BdkWindow       *window,
                                 gint            *x,
                                 gint            *y,
                                 BdkModifierType *mask)
{
  return bdk_directfb_window_get_pointer_helper (window, x, y, mask) != NULL;
}

BdkWindow *
_bdk_windowing_window_at_pointer (BdkDisplay      *display,
                                  gint            *win_x,
				  gint            *win_y,
                                  BdkModifierType *mask,
                                  gboolean         get_toplevel)
{
  BdkWindow *retval;
  gint       wx, wy;

  bdk_directfb_mouse_get_info (&wx, &wy, NULL);

  retval = bdk_directfb_child_at (_bdk_parent_root, &wx, &wy);

  if (win_x)
    *win_x = wx;

  if (win_y)
    *win_y = wy;

  if (get_toplevel)
    {
      BdkWindowObject *w = (BdkWindowObject *)retval;
      /* Requested toplevel, find it. */
      /* TODO: This can be implemented more efficient by never
	 recursing into children in the first place */
      if (w)
	{
	  /* Convert to toplevel */
	  while (w->parent != NULL &&
		 w->parent->window_type != BDK_WINDOW_ROOT)
	    {
	      *win_x += w->x;
	      *win_y += w->y;
	      w = w->parent;
	    }
	  retval = (BdkWindow *)w;
        }
    }

  return retval;
}

void
_bdk_windowing_get_pointer (BdkDisplay       *display,
                            BdkScreen       **screen,
                            gint             *x,
                            gint             *y,
                            BdkModifierType  *mask)
{
  if (screen) {
    *screen = bdk_display_get_default_screen (display);
  }

  bdk_directfb_window_get_pointer (_bdk_windowing_window_at_pointer (display,
                                                                     NULL,
                                                                     NULL,
                                                                     NULL,
                                                                     FALSE),
                                   x, y, mask);
}

static BdkEventMask
bdk_directfb_window_get_events (BdkWindow *window)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), 0);

  if (BDK_WINDOW_DESTROYED (window))
    return 0;
  else
    return BDK_WINDOW_OBJECT (window)->event_mask;
}

static void
bdk_directfb_window_set_events (BdkWindow    *window,
                                BdkEventMask  event_mask)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (event_mask & BDK_BUTTON_MOTION_MASK)
    event_mask |= (BDK_BUTTON1_MOTION_MASK |
                   BDK_BUTTON2_MOTION_MASK |
                   BDK_BUTTON3_MOTION_MASK);

  BDK_WINDOW_OBJECT (window)->event_mask = event_mask;
}

static void
bdk_directfb_window_shape_combine_rebunnyion (BdkWindow       *window,
                                          const BdkRebunnyion *shape_rebunnyion,
                                          gint             offset_x,
                                          gint             offset_y)
{
}

void
bdk_directfb_window_input_shape_combine_rebunnyion (BdkWindow       *window,
                                                const BdkRebunnyion *shape_rebunnyion,
                                                gint             offset_x,
                                                gint             offset_y)
{
}

static void
bdk_directfb_window_queue_translation (BdkWindow *window,
				       BdkGC     *gc,
                                       BdkRebunnyion *rebunnyion,
                                       gint       dx,
                                       gint       dy)
{
  BdkWindowObject         *private = BDK_WINDOW_OBJECT (window);
  BdkDrawableImplDirectFB *impl    = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, %p, %4d,%4d-%4d,%4d (%ld boxes), %d, %d )\n",
              G_STRFUNC, window, gc,
              BDKDFB_RECTANGLE_VALS_FROM_BOX (&rebunnyion->extents),
              rebunnyion->numRects, dx, dy);

  bdk_rebunnyion_offset (rebunnyion, dx, dy);
  bdk_rebunnyion_offset (rebunnyion, private->abs_x, private->abs_y);

  if (!impl->buffered)
    temp_rebunnyion_init_copy (&impl->paint_rebunnyion, rebunnyion);
  else
    bdk_rebunnyion_union (&impl->paint_rebunnyion, rebunnyion);
  impl->buffered = TRUE;

  bdk_rebunnyion_offset (rebunnyion, -dx, -dy);
  bdk_rebunnyion_offset (rebunnyion, -private->abs_x, -private->abs_y);
}

void
bdk_window_set_override_redirect (BdkWindow *window,
                                  gboolean   override_redirect)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_set_icon_list (BdkWindow *window,
                          GList     *pixbufs)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_set_icon (BdkWindow *window,
                     BdkWindow *icon_window,
                     BdkPixmap *pixmap,
                     BdkBitmap *mask)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_set_icon_name (BdkWindow   *window,
                          const gchar *name)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_iconify (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  bdk_window_hide (window);
}

void
bdk_window_deiconify (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  bdk_window_show (window);
}

void
bdk_window_stick (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_unstick (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_directfb_window_set_opacity (BdkWindow *window,
                                 guchar     opacity)
{
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  impl->opacity = opacity;

  if (impl->window && BDK_WINDOW_IS_MAPPED (window))
    {
      if (bdk_directfb_apply_focus_opacity &&
	  window == bdk_directfb_focused_window)
	impl->window->SetOpacity (impl->window,
				  (impl->opacity >> 1) + (impl->opacity >> 2));
      else
	impl->window->SetOpacity (impl->window, impl->opacity);
    }
}

void
bdk_window_focus (BdkWindow *window,
                  guint32    timestamp)
{
  BdkWindow *toplevel;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  toplevel = bdk_directfb_window_find_toplevel (window);
  if (toplevel != _bdk_parent_root)
    {
      BdkWindowImplDirectFB *impl;

      impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (toplevel)->impl);

      impl->window->RequestFocus (impl->window);
    }
}

void
bdk_window_maximize (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_unmaximize (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

void
bdk_window_set_type_hint (BdkWindow        *window,
                          BdkWindowTypeHint hint)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  BDK_NOTE (MISC, g_print ("bdk_window_set_type_hint: 0x%x: %d\n",
                           BDK_WINDOW_DFB_ID (window), hint));

  ((BdkWindowImplDirectFB *)((BdkWindowObject *)window)->impl)->type_hint = hint;


  /* N/A */
}

BdkWindowTypeHint
bdk_window_get_type_hint (BdkWindow *window)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), BDK_WINDOW_TYPE_HINT_NORMAL);

  if (BDK_WINDOW_DESTROYED (window))
    return BDK_WINDOW_TYPE_HINT_NORMAL;

  return BDK_WINDOW_IMPL_DIRECTFB (((BdkWindowObject *) window)->impl)->type_hint;
}

void
bdk_window_set_modal_hint (BdkWindow *window,
                           gboolean   modal)
{
  BdkWindowImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_WINDOW_IMPL_DIRECTFB (BDK_WINDOW_OBJECT (window)->impl);

  if (impl->window)
    {
      impl->window->SetStackingClass (impl->window,
                                      modal ? DWSC_UPPER : DWSC_MIDDLE);
    }
}

void
bdk_window_set_skip_taskbar_hint (BdkWindow *window,
				  gboolean   skips_taskbar)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
}

void
bdk_window_set_skip_pager_hint (BdkWindow *window,
				gboolean   skips_pager)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
}


void
bdk_window_set_group (BdkWindow *window,
                      BdkWindow *leader)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (BDK_IS_WINDOW (leader));
  g_warning (" DirectFb set_group groups not supported \n");

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
}

BdkWindow *
bdk_window_get_group (BdkWindow *window)
{
  g_warning (" DirectFb get_group groups not supported \n");
  return window;
}

void
bdk_fb_window_set_child_handler (BdkWindow             *window,
                                 BdkWindowChildChanged  changed,
                                 BdkWindowChildGetPos   get_pos,
                                 gpointer               user_data)
{
  BdkWindowChildHandlerData *data;

  g_return_if_fail (BDK_IS_WINDOW (window));

  data = g_new (BdkWindowChildHandlerData, 1);
  data->changed   = changed;
  data->get_pos   = get_pos;
  data->user_data = user_data;

  g_object_set_data_full (G_OBJECT (window), "bdk-window-child-handler",
                          data, (GDestroyNotify) g_free);
}

void
bdk_window_set_decorations (BdkWindow       *window,
                            BdkWMDecoration  decorations)
{
  BdkWMDecoration *dec;

  g_return_if_fail (BDK_IS_WINDOW (window));

  dec = g_new (BdkWMDecoration, 1);
  *dec = decorations;

  g_object_set_data_full (G_OBJECT (window), "bdk-window-decorations",
                          dec, (GDestroyNotify) g_free);
}

gboolean
bdk_window_get_decorations (BdkWindow       *window,
			    BdkWMDecoration *decorations)
{
  BdkWMDecoration *dec;

  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  dec = g_object_get_data (G_OBJECT (window), "bdk-window-decorations");
  if (dec)
    {
      *decorations = *dec;
      return TRUE;
    }
  return FALSE;
}

void
bdk_window_set_functions (BdkWindow     *window,
                          BdkWMFunction  functions)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  /* N/A */
  g_message ("unimplemented %s", G_STRFUNC);
}

static gboolean
bdk_directfb_window_set_static_gravities (BdkWindow *window,
                                          gboolean   use_static)
{
  g_return_val_if_fail (BDK_IS_WINDOW (window), FALSE);

  if (BDK_WINDOW_DESTROYED (window))
    return FALSE;

  /* N/A */
  g_message ("unimplemented %s", G_STRFUNC);

  return FALSE;
}

void
bdk_window_begin_resize_drag (BdkWindow     *window,
                              BdkWindowEdge  edge,
                              gint           button,
                              gint           root_x,
                              gint           root_y,
                              guint32        timestamp)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  g_message ("unimplemented %s", G_STRFUNC);
}

void
bdk_window_begin_move_drag (BdkWindow *window,
                            gint       button,
                            gint       root_x,
                            gint       root_y,
                            guint32    timestamp)
{
  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  g_message ("unimplemented %s", G_STRFUNC);
}

/**
 * bdk_window_get_frame_extents:
 * @window: a #BdkWindow
 * @rect: rectangle to fill with bounding box of the window frame
 *
 * Obtains the bounding box of the window, including window manager
 * titlebar/borders if any. The frame position is given in root window
 * coordinates. To get the position of the window itself (rather than
 * the frame) in root window coordinates, use bdk_window_get_origin().
 *
 **/
void
bdk_window_get_frame_extents (BdkWindow    *window,
                              BdkRectangle *rect)
{
  BdkWindowObject         *private;
  BdkDrawableImplDirectFB *impl;

  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (rect != NULL);

  if (BDK_WINDOW_DESTROYED (window))
    return;

  private = BDK_WINDOW_OBJECT (window);

  while (private->parent && ((BdkWindowObject*) private->parent)->parent)
    private = (BdkWindowObject*) private->parent;
  if (BDK_WINDOW_DESTROYED (window))
    return;

  impl = BDK_DRAWABLE_IMPL_DIRECTFB (private->impl);

  rect->x      = impl->abs_x;
  rect->y      = impl->abs_y;
  rect->width  = impl->width;
  rect->height = impl->height;
}

/*
 * Given a directfb window and a subsurface of that window
 * create a bdkwindow child wrapper
 */
BdkWindow *
bdk_directfb_create_child_window (BdkWindow *parent,
                                  IDirectFBSurface *subsurface)
{
  BdkWindow             *window;
  BdkWindowObject       *private;
  BdkWindowObject       *parent_private;
  BdkWindowImplDirectFB *impl;
  BdkWindowImplDirectFB *parent_impl;
  gint                   x, y, w, h;

  g_return_val_if_fail (parent != NULL, NULL);

  window          = g_object_new (BDK_TYPE_WINDOW, NULL);
  private         = BDK_WINDOW_OBJECT (window);
  private->impl   = g_object_new (_bdk_window_impl_get_type (), NULL);
  parent_private  = BDK_WINDOW_OBJECT (parent);
  parent_impl     = BDK_WINDOW_IMPL_DIRECTFB (parent_private->impl);
  private->parent = parent_private;

  subsurface->GetPosition (subsurface, &x, &y);
  subsurface->GetSize (subsurface, &w, &h);

  impl = BDK_WINDOW_IMPL_DIRECTFB (private->impl);
  impl->drawable.wrapper = BDK_DRAWABLE (window);

  private->x = x;
  private->y = y;

  private->abs_x = 0;
  private->abs_y = 0;

  impl->drawable.width     = w;
  impl->drawable.height    = h;
  private->window_type     = BDK_WINDOW_CHILD;
  impl->drawable.surface   = subsurface;
  impl->drawable.format    = parent_impl->drawable.format;
  private->depth           = parent_private->depth;
  bdk_drawable_set_colormap (BDK_DRAWABLE (window),
                             bdk_drawable_get_colormap (parent));
  bdk_window_set_cursor (window, NULL);
  parent_private->children = g_list_prepend (parent_private->children,window);
  /*we hold a reference count on ourselves */
  g_object_ref (window);

  return window;

}

/*
 * The wrapping is not perfect since directfb does not give full access
 * to the current state of a window event mask etc need to fix dfb
 */
BdkWindow *
bdk_window_foreign_new_for_display (BdkDisplay* display, BdkNativeWindow anid)
{
  BdkWindow             *window         = NULL;
  BdkWindow             *parent         = NULL;
  BdkWindowObject       *private        = NULL;
  BdkWindowObject       *parent_private = NULL;
  BdkWindowImplDirectFB *parent_impl    = NULL;
  BdkWindowImplDirectFB *impl           = NULL;
  DFBWindowOptions       options;
  DFBResult              ret;
  BdkDisplayDFB *        bdkdisplay     = _bdk_display;
  IDirectFBWindow       *dfbwindow;

  window = bdk_window_lookup (anid);

  if (window) {
    g_object_ref (window);
    return window;
  }
  if (display != NULL)
    bdkdisplay = BDK_DISPLAY_DFB (display);

  ret = bdkdisplay->layer->GetWindow (bdkdisplay->layer,
                                      (DFBWindowID)anid,
                                      &dfbwindow);

  if (ret != DFB_OK) {
    DirectFBError ("bdk_window_new: Layer->GetWindow failed", ret);
    return NULL;
  }

  parent = _bdk_parent_root;

  if (parent) {
    parent_private = BDK_WINDOW_OBJECT (parent);
    parent_impl = BDK_WINDOW_IMPL_DIRECTFB (parent_private->impl);
  }

  window = g_object_new (BDK_TYPE_WINDOW, NULL);
  /* we hold a reference count on ourselves */
  g_object_ref (window);
  private              = BDK_WINDOW_OBJECT (window);
  private->impl        = g_object_new (_bdk_window_impl_get_type (), NULL);
  private->parent      = parent_private;
  private->window_type = BDK_WINDOW_TOPLEVEL;
  private->viewable    = TRUE;
  impl                 = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  impl->drawable.wrapper = BDK_DRAWABLE (window);
  impl->window           = dfbwindow;
  dfbwindow->GetOptions (dfbwindow, &options);
  dfbwindow->GetPosition (dfbwindow, &private->x, &private->y);
  dfbwindow->GetSize (dfbwindow, &impl->drawable.width, &impl->drawable.height);


  private->input_only = FALSE;

  if (dfbwindow->GetSurface (dfbwindow,
                             &impl->drawable.surface) == DFB_UNSUPPORTED) {
    private->input_only    = TRUE;
    impl->drawable.surface = NULL;
  }

  /* We default to all events least surprise to the user
   * minus the poll for motion events
   */
  bdk_window_set_events (window, (BDK_ALL_EVENTS_MASK ^ BDK_POINTER_MOTION_HINT_MASK));

  if (impl->drawable.surface)
    {
      impl->drawable.surface->GetPixelFormat (impl->drawable.surface,
					      &impl->drawable.format);

      private->depth = DFB_BITS_PER_PIXEL(impl->drawable.format);
      if (parent)
        bdk_drawable_set_colormap (BDK_DRAWABLE (window), bdk_drawable_get_colormap (parent));
      else
        bdk_drawable_set_colormap (BDK_DRAWABLE (window), bdk_colormap_get_system ());
    }

  //can  be null for the soft cursor window itself when
  //running a btk directfb wm
  if (bdk_display_get_default () != NULL) {
    bdk_window_set_cursor (window, NULL);
  }

  if (parent_private)
    parent_private->children = g_list_prepend (parent_private->children,
                                               window);
  impl->dfb_id = (DFBWindowID)anid;
  bdk_directfb_window_id_table_insert (impl->dfb_id, window);
  bdk_directfb_event_windows_add (window);

  return window;
}

BdkWindow *
bdk_window_lookup_for_display (BdkDisplay *display,BdkNativeWindow anid)
{
  return bdk_directfb_window_id_table_lookup ((DFBWindowID) anid);
}

BdkWindow *
bdk_window_lookup (BdkNativeWindow anid)
{
  return bdk_directfb_window_id_table_lookup ((DFBWindowID) anid);
}

IDirectFBWindow *
bdk_directfb_window_lookup (BdkWindow *window)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;

  g_return_val_if_fail (BDK_IS_WINDOW (window), NULL);

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  return impl->window;
}

IDirectFBSurface *
bdk_directfb_surface_lookup (BdkWindow *window)
{
  BdkWindowObject       *private;
  BdkWindowImplDirectFB *impl;

  g_return_val_if_fail (BDK_IS_WINDOW (window),NULL);

  private = BDK_WINDOW_OBJECT (window);
  impl    = BDK_WINDOW_IMPL_DIRECTFB (private->impl);

  return impl->drawable.surface;
}

void
bdk_window_fullscreen (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  g_warning ("bdk_window_fullscreen() not implemented.\n");
}

void
bdk_window_unfullscreen (BdkWindow *window)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  /* g_warning ("bdk_window_unfullscreen() not implemented.\n");*/
}

void
bdk_window_set_keep_above (BdkWindow *window, gboolean setting)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  static gboolean first_call = TRUE;
  if (first_call) {
    g_warning ("bdk_window_set_keep_above() not implemented.\n");
    first_call=FALSE;
  }

}

void
bdk_window_set_keep_below (BdkWindow *window, gboolean setting)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  static gboolean first_call = TRUE;
  if (first_call) {
    g_warning ("bdk_window_set_keep_below() not implemented.\n");
    first_call=FALSE;
  }

}

void
bdk_window_enable_synchronized_configure (BdkWindow *window)
{
}

void
bdk_window_configure_finished (BdkWindow *window)
{
}

void
bdk_display_warp_pointer (BdkDisplay *display,
                          BdkScreen  *screen,
                          gint        x,
                          gint        y)
{
  g_warning ("bdk_display_warp_pointer() not implemented.\n");
}

void
bdk_window_set_urgency_hint (BdkWindow *window,
                             gboolean   urgent)
{
  g_return_if_fail (BDK_IS_WINDOW (window));
  g_return_if_fail (BDK_WINDOW_TYPE (window) != BDK_WINDOW_CHILD);

  if (BDK_WINDOW_DESTROYED (window))
    return;

  g_warning ("bdk_window_set_urgency_hint() not implemented.\n");

}

static void
bdk_window_impl_directfb_begin_paint_rebunnyion (BdkPaintable    *paintable,
                                             BdkWindow       *window,
                                             const BdkRebunnyion *rebunnyion)
{
  BdkWindowObject         *private = BDK_WINDOW_OBJECT (window);
  /* BdkWindowImplDirectFB   *wimpl   = BDK_WINDOW_IMPL_DIRECTFB (paintable); */
  BdkDrawableImplDirectFB *impl    = BDK_DRAWABLE_IMPL_DIRECTFB (paintable);
  BdkRebunnyion               *native_rebunnyion;
  gint                     i;

  g_assert (rebunnyion != NULL);

  D_DEBUG_AT (BDKDFB_Window, "%s( %p, %p, %4d,%4d-%4d,%4d (%ld boxes) )\n",
              G_STRFUNC, paintable, window,
              BDKDFB_RECTANGLE_VALS_FROM_BOX (&rebunnyion->extents),
              rebunnyion->numRects);
  D_DEBUG_AT (BDKDFB_Window, "  -> window @ pos=%ix%i abs_pos=%ix%i\n",
              private->x, private->y, private->abs_x, private->abs_y);

  native_rebunnyion = bdk_rebunnyion_copy (rebunnyion);
  bdk_rebunnyion_offset (native_rebunnyion, private->abs_x, private->abs_y);

  /* /\* When it's buffered... *\/ */
  if (impl->buffered)
    {
      /* ...we're already painting on it! */
      D_DEBUG_AT (BDKDFB_Window, "  -> painted  %4d,%4d-%4dx%4d (%ld boxes)\n",
                  DFB_RECTANGLE_VALS_FROM_REBUNNYION (&impl->paint_rebunnyion.extents),
                  impl->paint_rebunnyion.numRects);

      if (impl->paint_depth < 1)
        bdk_directfb_clip_rebunnyion (BDK_DRAWABLE (paintable),
                                  NULL, NULL, &impl->clip_rebunnyion);

      bdk_rebunnyion_union (&impl->paint_rebunnyion, native_rebunnyion);
    }
  else
    {
      /* ...otherwise it's the first time! */
      g_assert (impl->paint_depth == 0);

      /* Generate the clip rebunnyion for painting around child windows. */
      bdk_directfb_clip_rebunnyion (BDK_DRAWABLE (paintable),
                                NULL, NULL, &impl->clip_rebunnyion);

      /* Initialize the paint rebunnyion with the new one... */
      temp_rebunnyion_init_copy (&impl->paint_rebunnyion, native_rebunnyion);

      impl->buffered = TRUE;
    }

  D_DEBUG_AT (BDKDFB_Window, "  -> painting %4d,%4d-%4dx%4d (%ld boxes)\n",
              DFB_RECTANGLE_VALS_FROM_REBUNNYION (&impl->paint_rebunnyion.extents),
              impl->paint_rebunnyion.numRects);

  /* ...but clip the initial/compound result against the clip rebunnyion. */
  /* bdk_rebunnyion_intersect (&impl->paint_rebunnyion, &impl->clip_rebunnyion); */

  D_DEBUG_AT (BDKDFB_Window, "  -> clipped  %4d,%4d-%4dx%4d (%ld boxes)\n",
              DFB_RECTANGLE_VALS_FROM_REBUNNYION (&impl->paint_rebunnyion.extents),
              impl->paint_rebunnyion.numRects);

  impl->paint_depth++;

  D_DEBUG_AT (BDKDFB_Window, "  -> depth is now %d\n", impl->paint_depth);

  /*
   * Redraw background on area which are going to be repainted.
   *
   * TODO: handle pixmap background
   */
  impl->surface->SetClip (impl->surface, NULL);
  for (i = 0 ; i < native_rebunnyion->numRects ; i++)
    {
      BdkRebunnyionBox *box = &native_rebunnyion->rects[i];

      D_DEBUG_AT (BDKDFB_Window, "  -> clearing [%2d] %4d,%4d-%4dx%4d\n",
                  i, BDKDFB_RECTANGLE_VALS_FROM_BOX (box));

      /* bdk_window_clear_area (window, */
      /*                        box->x1, */
      /*                        box->y1, */
      /*                        box->x2 - box->x1, */
      /*                        box->y2 - box->y1); */

      impl->surface->SetColor (impl->surface,
                               private->bg_color.red,
                               private->bg_color.green,
                               private->bg_color.blue,
                               0xff);
      impl->surface->FillRectangle (impl->surface,
                                    box->x1,
                                    box->y1,
                                    box->x2 - box->x1,
                                    box->y2 - box->y1);
    }

  bdk_rebunnyion_destroy (native_rebunnyion);
}

static void
bdk_window_impl_directfb_end_paint (BdkPaintable *paintable)
{
  BdkDrawableImplDirectFB *impl;

  impl = BDK_DRAWABLE_IMPL_DIRECTFB (paintable);

  D_DEBUG_AT (BDKDFB_Window, "%s( %p )\n", G_STRFUNC, paintable);

  g_return_if_fail (impl->paint_depth > 0);

  g_assert (impl->buffered);

  impl->paint_depth--;

#ifdef BDK_DIRECTFB_NO_EXPERIMENTS
  if (impl->paint_depth == 0)
    {
      impl->buffered = FALSE;

      if (impl->paint_rebunnyion.numRects)
        {
          DFBRebunnyion reg = { impl->paint_rebunnyion.extents.x1,
                            impl->paint_rebunnyion.extents.y1,
                            impl->paint_rebunnyion.extents.x2 - 1,
                            impl->paint_rebunnyion.extents.y2 - 1 };

          D_DEBUG_AT (BDKDFB_Window, "  -> flip %4d,%4d-%4dx%4d (%ld boxes)\n",
                      DFB_RECTANGLE_VALS_FROM_REBUNNYION (&reg),
                      impl->paint_rebunnyion.numRects);

          impl->surface->Flip (impl->surface, &reg, 0);

          temp_rebunnyion_reset (&impl->paint_rebunnyion);
        }
    }
#else
  if (impl->paint_depth == 0)
    {
      impl->buffered = FALSE;

      temp_rebunnyion_deinit (&impl->clip_rebunnyion);

      if (impl->paint_rebunnyion.numRects)
        {
          BdkWindow *window = BDK_WINDOW (impl->wrapper);

          if (BDK_IS_WINDOW (window))
            {
              BdkWindowObject *top = BDK_WINDOW_OBJECT (bdk_window_get_toplevel (window));

              if (top)
                {
                  DFBRebunnyion              reg;
                  BdkWindowImplDirectFB *wimpl = BDK_WINDOW_IMPL_DIRECTFB (top->impl);

                  reg.x1 = impl->abs_x - top->x + impl->paint_rebunnyion.extents.x1;
                  reg.y1 = impl->abs_y - top->y + impl->paint_rebunnyion.extents.y1;
                  reg.x2 = impl->abs_x - top->x + impl->paint_rebunnyion.extents.x2 - 1;
                  reg.y2 = impl->abs_y - top->y + impl->paint_rebunnyion.extents.y2 - 1;

                  D_DEBUG_AT (BDKDFB_Window, "  -> queue flip %4d,%4d-%4dx%4d (%ld boxes)\n",
                              DFB_RECTANGLE_VALS_FROM_REBUNNYION (&reg),
                              impl->paint_rebunnyion.numRects);

                  dfb_updates_add (&wimpl->flips, &reg);
                }
            }

          temp_rebunnyion_reset (&impl->paint_rebunnyion);
        }
    }
#endif
  else
    D_DEBUG_AT (BDKDFB_Window, "  -> depth is still %d\n", impl->paint_depth);
}

BdkRebunnyion *
_bdk_windowing_get_shape_for_mask (BdkBitmap *mask)
{
  return NULL;
}

BdkRebunnyion *
_bdk_windowing_window_get_shape (BdkWindow *window)
{
  return NULL;
}

gulong
_bdk_windowing_window_get_next_serial (BdkDisplay *display)
{
  return 0;
}

BdkRebunnyion *
_bdk_windowing_window_get_input_shape (BdkWindow *window)
{
  return NULL;
}

void
_bdk_windowing_before_process_all_updates (void)
{
}

void
_bdk_windowing_after_process_all_updates (void)
{
}

void
_bdk_windowing_window_process_updates_recurse (BdkWindow *window,
                                               BdkRebunnyion *rebunnyion)
{
  _bdk_window_process_updates_recurse (window, rebunnyion);
}

static void
bdk_window_impl_directfb_paintable_init (BdkPaintableIface *iface)
{
  iface->begin_paint_rebunnyion = bdk_window_impl_directfb_begin_paint_rebunnyion;
  iface->end_paint          = bdk_window_impl_directfb_end_paint;
}

void
_bdk_windowing_window_beep (BdkWindow *window)
{
  bdk_display_beep (bdk_display_get_default());
}

void
bdk_window_set_opacity (BdkWindow *window,
			gdouble    opacity)
{
  BdkDisplay *display;
  guint8 cardinal;

  g_return_if_fail (BDK_IS_WINDOW (window));

  if (BDK_WINDOW_DESTROYED (window))
    return;

  display = bdk_drawable_get_display (window);

  if (opacity < 0)
    opacity = 0;
  else if (opacity > 1)
    opacity = 1;
  cardinal = opacity * 0xff;
  bdk_directfb_window_set_opacity (window, cardinal);
}

void
_bdk_windowing_window_set_composited (BdkWindow *window,
                                      gboolean   composited)
{
}

static gint
bdk_directfb_window_get_root_coords (BdkWindow *window,
                                     gint       x,
                                     gint       y,
                                     gint      *root_x,
                                     gint      *root_y)
{
  /* TODO */
  return 1;
}

static gboolean
bdk_directfb_window_queue_antiexpose (BdkWindow *window,
                                      BdkRebunnyion *area)
{
  return FALSE;
}

static void
bdk_window_impl_iface_init (BdkWindowImplIface *iface)
{
  iface->show                       = bdk_directfb_window_show;
  iface->hide                       = bdk_directfb_window_hide;
  iface->withdraw                   = bdk_directfb_window_withdraw;
  iface->set_events                 = bdk_directfb_window_set_events;
  iface->get_events                 = bdk_directfb_window_get_events;
  iface->raise                      = bdk_window_directfb_raise;
  iface->lower                      = bdk_window_directfb_lower;
  iface->move_resize                = bdk_directfb_window_move_resize;
  iface->set_background             = bdk_directfb_window_set_background;
  iface->set_back_pixmap            = bdk_directfb_window_set_back_pixmap;
  iface->reparent                   = bdk_directfb_window_reparent;
  iface->set_cursor                 = bdk_directfb_window_set_cursor;
  iface->get_geometry               = bdk_directfb_window_get_geometry;
  iface->get_root_coords            = bdk_directfb_window_get_root_coords;
  iface->get_pointer                = bdk_directfb_window_get_pointer;
  iface->get_deskrelative_origin    = bdk_directfb_window_get_deskrelative_origin;
  iface->shape_combine_rebunnyion       = bdk_directfb_window_shape_combine_rebunnyion;
  iface->input_shape_combine_rebunnyion = bdk_directfb_window_input_shape_combine_rebunnyion;
  iface->set_static_gravities       = bdk_directfb_window_set_static_gravities;
  iface->queue_antiexpose           = bdk_directfb_window_queue_antiexpose;
  iface->queue_translation          = bdk_directfb_window_queue_translation;
  iface->destroy                    = bdk_directfb_window_destroy;
}

#define __BDK_WINDOW_X11_C__
#include "bdkaliasdef.c"

