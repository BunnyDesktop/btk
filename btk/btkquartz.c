/* btkquartz.c: Utility functions used by the Quartz port
 *
 * Copyright (C) 2006 Imendio AB
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

#include "btkquartz.h"
#include <bdk/quartz/bdkquartz.h>
#include "btkalias.h"

NSImage *
_btk_quartz_create_image_from_pixbuf (BdkPixbuf *pixbuf)
{
  CGColorSpaceRef colorspace;
  CGDataProviderRef data_provider;
  CGContextRef context;
  CGImageRef image;
  void *data;
  int rowstride, pixbuf_width, pixbuf_height;
  bboolean has_alpha;
  NSImage *nsimage;
  NSSize nsimage_size;

  pixbuf_width = bdk_pixbuf_get_width (pixbuf);
  pixbuf_height = bdk_pixbuf_get_height (pixbuf);
  g_return_val_if_fail (pixbuf_width != 0 && pixbuf_height != 0, NULL);
  rowstride = bdk_pixbuf_get_rowstride (pixbuf);
  has_alpha = bdk_pixbuf_get_has_alpha (pixbuf);

  data = bdk_pixbuf_get_pixels (pixbuf);

  colorspace = CGColorSpaceCreateDeviceRGB ();
  data_provider = CGDataProviderCreateWithData (NULL, data, pixbuf_height * rowstride, NULL);

  image = CGImageCreate (pixbuf_width, pixbuf_height, 8,
			 has_alpha ? 32 : 24, rowstride, 
			 colorspace, 
			 has_alpha ? kCGImageAlphaLast : 0,
			 data_provider, NULL, FALSE, 
			 kCGRenderingIntentDefault);

  CGDataProviderRelease (data_provider);
  CGColorSpaceRelease (colorspace);

  nsimage = [[NSImage alloc] initWithSize:NSMakeSize (pixbuf_width, pixbuf_height)];
  nsimage_size = [nsimage size];
  if (nsimage_size.width == 0.0 && nsimage_size.height == 0.0)
    {
      [nsimage release];
      g_return_val_if_fail (FALSE, NULL);
    }
  [nsimage lockFocus];

  context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  CGContextDrawImage (context, CGRectMake (0, 0, pixbuf_width, pixbuf_height), image);
 
  [nsimage unlockFocus];

  CGImageRelease (image);

  return nsimage;
}

NSSet *
_btk_quartz_target_list_to_pasteboard_types (BtkTargetList *target_list)
{
  NSMutableSet *set = [[NSMutableSet alloc] init];
  GList *list;

  for (list = target_list->list; list; list = list->next)
    {
      BtkTargetPair *pair = list->data;
      [set addObject:bdk_quartz_atom_to_pasteboard_type_libbtk_only (pair->target)];
    }

  return set;
}

NSSet *
_btk_quartz_target_entries_to_pasteboard_types (const BtkTargetEntry *targets,
						buint                 n_targets)
{
  NSMutableSet *set = [[NSMutableSet alloc] init];
  int i;

  for (i = 0; i < n_targets; i++)
    {
      [set addObject:bdk_quartz_target_to_pasteboard_type_libbtk_only (targets[i].target)];
    }

  return set;
}

GList *
_btk_quartz_pasteboard_types_to_atom_list (NSArray *array)
{
  GList *result = NULL;
  int i;
  int count;

  count = [array count];

  for (i = 0; i < count; i++) 
    {
      BdkAtom atom = bdk_quartz_pasteboard_type_to_atom_libbtk_only ([array objectAtIndex:i]);

      result = g_list_prepend (result, BDK_ATOM_TO_POINTER (atom));
    }

  return result;
}

BtkSelectionData *
_btk_quartz_get_selection_data_from_pasteboard (NSPasteboard *pasteboard,
						BdkAtom       target,
						BdkAtom       selection)
{
  BtkSelectionData *selection_data = NULL;

  selection_data = g_slice_new0 (BtkSelectionData);
  selection_data->selection = selection;
  selection_data->target = target;
  if (!selection_data->display)
    selection_data->display = bdk_display_get_default ();
  if (target == bdk_atom_intern_static_string ("UTF8_STRING"))
    {
      NSString *s = [pasteboard stringForType:NSStringPboardType];

      if (s)
	{
          const char *utf8_string = [s UTF8String];

          btk_selection_data_set (selection_data,
                                  target, 8,
                                  (buchar *)utf8_string, strlen (utf8_string));
	}
    }
  else if (target == bdk_atom_intern_static_string ("application/x-color"))
    {
      NSColor *nscolor = [[NSColor colorFromPasteboard:pasteboard]
                          colorUsingColorSpaceName:NSDeviceRGBColorSpace];
      
      buint16 color[4];
      
      selection_data->target = target;

      color[0] = 0xffff * [nscolor redComponent];
      color[1] = 0xffff * [nscolor greenComponent];
      color[2] = 0xffff * [nscolor blueComponent];
      color[3] = 0xffff * [nscolor alphaComponent];

      btk_selection_data_set (selection_data, target, 16, (buchar *)color, 8);
    }
  else if (target == bdk_atom_intern_static_string ("text/uri-list"))
    {
      if ([[pasteboard types] containsObject:NSFilenamesPboardType])
        {
           bchar **uris;
           NSArray *files = [pasteboard propertyListForType:NSFilenamesPboardType];
           int n_files = [files count];
           int i;

           selection_data->target = bdk_atom_intern_static_string ("text/uri-list");

           uris = (bchar **) g_malloc (sizeof (bchar*) * (n_files + 1));
           for (i = 0; i < n_files; ++i)
             {
               NSString* uriString = [files objectAtIndex:i];
               uriString = [@"file://" stringByAppendingString:uriString];
               uriString = [uriString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
               uris[i] = (bchar *) [uriString cStringUsingEncoding:NSUTF8StringEncoding];
             }
           uris[i] = NULL;

           btk_selection_data_set_uris (selection_data, uris);
           g_free (uris);
         }
      else if ([[pasteboard types] containsObject:NSURLPboardType])
        {
          bchar *uris[2];
          NSURL *url = [NSURL URLFromPasteboard:pasteboard];

          selection_data->target = bdk_atom_intern_static_string ("text/uri-list");

          uris[0] = (bchar *) [[url description] UTF8String];

          uris[1] = NULL;
          btk_selection_data_set_uris (selection_data, uris);
        }
    }
  else
    {
      NSData *data;
      bchar *name;

      name = bdk_atom_name (target);

      if (strcmp (name, "image/tiff") == 0)
	data = [pasteboard dataForType:NSTIFFPboardType];
      else
	data = [pasteboard dataForType:[NSString stringWithUTF8String:name]];

      g_free (name);

      if (data)
	{
	  btk_selection_data_set (selection_data,
                                  target, 8,
                                  [data bytes], [data length]);
	}
    }

  return selection_data;
}

void
_btk_quartz_set_selection_data_for_pasteboard (NSPasteboard     *pasteboard,
					       BtkSelectionData *selection_data)
{
  NSString *type;
  BdkDisplay *display;
  bint format;
  const buchar *data;
  NSUInteger length;

  display = btk_selection_data_get_display (selection_data);
  format = btk_selection_data_get_format (selection_data);
  data = btk_selection_data_get_data (selection_data);
  length = btk_selection_data_get_length (selection_data);

  type = bdk_quartz_atom_to_pasteboard_type_libbtk_only (btk_selection_data_get_target (selection_data));

  if ([type isEqualTo:NSStringPboardType]) 
    [pasteboard setString:[NSString stringWithUTF8String:(const char *)data]
                  forType:type];
  else if ([type isEqualTo:NSColorPboardType])
    {
      buint16 *color = (buint16 *)data;
      float red, green, blue, alpha;
      NSColor *nscolor;

      red   = (float)color[0] / 0xffff;
      green = (float)color[1] / 0xffff;
      blue  = (float)color[2] / 0xffff;
      alpha = (float)color[3] / 0xffff;

      nscolor = [NSColor colorWithDeviceRed:red green:green blue:blue alpha:alpha];
      [nscolor writeToPasteboard:pasteboard];
    }
  else if ([type isEqualTo:NSURLPboardType])
    {
      bchar **list = NULL;
      int count;

      count = bdk_text_property_to_utf8_list_for_display (display,
                                                          bdk_atom_intern_static_string ("UTF8_STRING"),
                                                          format,
                                                          data,
                                                          length,
                                                          &list);

      if (count > 0)
        {
          bchar **result;
          NSURL *url;

          result = g_uri_list_extract_uris (list[0]);

          url = [NSURL URLWithString:[NSString stringWithUTF8String:result[0]]];
          [url writeToPasteboard:pasteboard];

          g_strfreev (result);
        }

      g_strfreev (list);
    }
  else
    [pasteboard setData:[NSData dataWithBytesNoCopy:(void *)data
                                             length:length
                                       freeWhenDone:NO]
                                            forType:type];
}

/*
 * Bundle-based functions for various directories. These almost work
 * even when the application isn't in a bundle, becuase mainBundle
 * paths point to the bin directory in that case. It's a simple matter
 * to test for that and remove the last element.
 */

static const bchar *
get_bundle_path (void)
{
  static bchar *path = NULL;

  if (path == NULL)
    {
      NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
      bchar *resource_path = g_strdup ([[[NSBundle mainBundle] resourcePath] UTF8String]);
      bchar *base;
      [pool drain];

      base = g_path_get_basename (resource_path);
      if (strcmp (base, "bin") == 0)
	path = g_path_get_dirname (resource_path);
      else
	path = strdup (resource_path);

      g_free (resource_path);
      g_free (base);
    }

  return path;
}

const bchar *
_btk_get_datadir (void)
{
  static bchar *path = NULL;

  if (path == NULL)
    path = g_build_filename (get_bundle_path (), "share", NULL);

  return path;
}

const bchar *
_btk_get_libdir (void)
{
  static bchar *path = NULL;

  if (path == NULL)
    path = g_build_filename (get_bundle_path (), "lib", NULL);

  return path;
}

const bchar *
_btk_get_localedir (void)
{
  static bchar *path = NULL;

  if (path == NULL)
    path = g_build_filename (get_bundle_path (), "share", "locale", NULL);

  return path;
}

const bchar *
_btk_get_sysconfdir (void)
{
  static bchar *path = NULL;

  if (path == NULL)
    path = g_build_filename (get_bundle_path (), "etc", NULL);

  return path;
}

const bchar *
_btk_get_data_prefix (void)
{
  return get_bundle_path ();
}
