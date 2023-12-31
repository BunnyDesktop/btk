/* btkstatusicon-quartz.c:
 *
 * Copyright (C) 2006 Imendio AB
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
 *
 * GCC on Mac OS X handles inlined objective C in C-files.
 *
 * Authors:
 *  Mikael Hallendal <micke@imendio.com>
 */

#import <Cocoa/Cocoa.h>
#include <quartz/bdkquartz.h>

#define QUARTZ_POOL_ALLOC NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init]
#define QUARTZ_POOL_RELEASE [pool release]

@interface BtkQuartzStatusIcon : NSObject 
{
  BtkStatusIcon *status_icon;
  NSStatusBar   *ns_bar;
  NSStatusItem  *ns_item;
  NSImage       *current_image;
  NSString      *ns_tooltip;
}
- (id) initWithStatusIcon:(BtkStatusIcon *)status_icon;
- (void) ensureItem;
- (void) actionCb:(NSObject *)button;
- (void) setImage:(BdkPixbuf *)pixbuf;
- (void) setVisible:(bboolean)visible;
- (void) setToolTip:(const bchar *)tooltip_text;
- (float) getWidth;
- (float) getHeight;
@end

@implementation BtkQuartzStatusIcon : NSObject
- (id) initWithStatusIcon:(BtkStatusIcon *)icon
{
  [super init];
  status_icon = icon;
  ns_bar = [NSStatusBar systemStatusBar];

  return self;
}

- (void) ensureItem
{
  if (ns_item != nil)
    return;

  ns_item = [ns_bar statusItemWithLength:NSVariableStatusItemLength];
  [ns_item setAction:@selector(actionCb:)];
  [ns_item setTarget:self];
  [ns_item retain];
}

- (void) dealloc
{
  g_print ("Deallocating BtkQuartzStatusIcon\n");
  [current_image release];
  [ns_item release];
  [ns_bar release];

  [super dealloc];
}

- (void) actionCb:(NSObject *)button
{ 
  NSEvent *event = [NSApp currentEvent];
  double time = [event timestamp];
  
  g_signal_emit (status_icon,
                 status_icon_signals [POPUP_MENU_SIGNAL], 0,
                 1,
                 time * 1000.0);
}

- (void) setImage:(BdkPixbuf *)pixbuf
{
  /* Support NULL */
  [self ensureItem];

  if (current_image != nil) {
    [current_image release];
    current_image = nil;
  }
  
  if (!pixbuf) {
    [ns_item release];
    ns_item = nil;
    return;
  }

  current_image = bdk_quartz_pixbuf_to_ns_image_libbtk_only (pixbuf);
  [current_image retain];

  [ns_item setImage:current_image];
}

- (void) setVisible:(bboolean)visible
{
  if (visible) {
    [self ensureItem];
    if (ns_item != nil)
      [ns_item setImage:current_image];
    if (ns_tooltip != nil)
      [ns_item setToolTip:ns_tooltip];
  } else {
    [ns_item release];
    ns_item = nil;
  }
}

- (void) setToolTip:(const bchar *)tooltip_text
{
  [ns_tooltip release];
  ns_tooltip = [[NSString stringWithUTF8String:tooltip_text] retain];
  
  [ns_item setToolTip:ns_tooltip];
}

- (float) getWidth
{
  return [ns_bar thickness];
}

- (float) getHeight
{
  return [ns_bar thickness];
}
@end



