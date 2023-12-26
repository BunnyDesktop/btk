/* BdkQuartzView.m
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#import "BdkQuartzView.h"
#include "bdkrebunnyion.h"
#include "bdkrebunnyion-generic.h"
#include "bdkwindow-quartz.h"
#include "bdkprivate-quartz.h"
#include "bdkquartz.h"

@implementation BdkQuartzView

-(id)initWithFrame: (NSRect)frameRect
{
  if ((self = [super initWithFrame: frameRect]))
    {
      markedRange = NSMakeRange (NSNotFound, 0);
      selectedRange = NSMakeRange (NSNotFound, 0);
    }

  return self;
}

-(BOOL)acceptsFirstResponder
{
  BDK_NOTE (EVENTS, g_print ("acceptsFirstResponder\n"));
  return YES;
}

-(BOOL)becomeFirstResponder
{
  BDK_NOTE (EVENTS, g_print ("becomeFirstResponder\n"));
  return YES;
}

-(BOOL)resignFirstResponder
{
  BDK_NOTE (EVENTS, g_print ("resignFirstResponder\n"));
  return YES;
}

-(void) keyDown: (NSEvent *) theEvent
{
  BDK_NOTE (EVENTS, g_print ("keyDown\n"));
  [self interpretKeyEvents: [NSArray arrayWithObject: theEvent]];
}

-(void)flagsChanged: (NSEvent *) theEvent
{
}

-(NSUInteger)characterIndexForPoint: (NSPoint)aPoint
{
  BDK_NOTE (EVENTS, g_print ("characterIndexForPoint\n"));
  return 0;
}

-(NSRect)firstRectForCharacterRange: (NSRange)aRange actualRange: (NSRangePointer)actualRange
{
  BDK_NOTE (EVENTS, g_print ("firstRectForCharacterRange\n"));
  bint ns_x, ns_y;
  BdkRectangle *rect;

  rect = g_object_get_data (B_OBJECT (bdk_window), GIC_CURSOR_RECT);
  if (rect)
    {
      _bdk_quartz_window_bdk_xy_to_xy (rect->x, rect->y + rect->height,
				       &ns_x, &ns_y);

      return NSMakeRect (ns_x, ns_y, rect->width, rect->height);
    }
  else
    {
      return NSMakeRect (0, 0, 0, 0);
    }
}

-(NSArray *)validAttributesForMarkedText
{
  BDK_NOTE (EVENTS, g_print ("validAttributesForMarkedText\n"));
  return [NSArray arrayWithObjects: NSUnderlineStyleAttributeName, nil];
}

-(NSAttributedString *)attributedSubstringForProposedRange: (NSRange)aRange actualRange: (NSRangePointer)actualRange
{
  BDK_NOTE (EVENTS, g_print ("attributedSubstringForProposedRange\n"));
  return nil;
}

-(BOOL)hasMarkedText
{
  BDK_NOTE (EVENTS, g_print ("hasMarkedText\n"));
  return markedRange.location != NSNotFound && markedRange.length != 0;
}

-(NSRange)markedRange
{
  BDK_NOTE (EVENTS, g_print ("markedRange\n"));
  return markedRange;
}

-(NSRange)selectedRange
{
  BDK_NOTE (EVENTS, g_print ("selectedRange\n"));
  return selectedRange;
}

-(void)unmarkText
{
  BDK_NOTE (EVENTS, g_print ("unmarkText\n"));
  bchar *prev_str;
  markedRange = selectedRange = NSMakeRange (NSNotFound, 0);

  prev_str = g_object_get_data (B_OBJECT (bdk_window), TIC_MARKED_TEXT);
  if (prev_str)
    g_free (prev_str);
  g_object_set_data (B_OBJECT (bdk_window), TIC_MARKED_TEXT, NULL);
}

-(void)setMarkedText: (id)aString selectedRange: (NSRange)newSelection replacementRange: (NSRange)replacementRange
{
  BDK_NOTE (EVENTS, g_print ("setMarkedText\n"));
  const char *str;
  bchar *prev_str;

  if (replacementRange.location == NSNotFound)
    {
      markedRange = NSMakeRange (newSelection.location, [aString length]);
      selectedRange = NSMakeRange (newSelection.location, newSelection.length);
    }
  else {
      markedRange = NSMakeRange (replacementRange.location, [aString length]);
      selectedRange = NSMakeRange (replacementRange.location + newSelection.location, newSelection.length);
    }

  if ([aString isKindOfClass: [NSAttributedString class]])
    {
      str = [[aString string] UTF8String];
    }
  else {
      str = [aString UTF8String];
    }

  prev_str = g_object_get_data (B_OBJECT (bdk_window), TIC_MARKED_TEXT);
  if (prev_str)
    g_free (prev_str);
  g_object_set_data (B_OBJECT (bdk_window), TIC_MARKED_TEXT, g_strdup (str));
  g_object_set_data (B_OBJECT (bdk_window), TIC_SELECTED_POS,
		     BUINT_TO_POINTER (selectedRange.location));
  g_object_set_data (B_OBJECT (bdk_window), TIC_SELECTED_LEN,
		     BUINT_TO_POINTER (selectedRange.length));

  BDK_NOTE (EVENTS, g_print ("setMarkedText: set %s (%p, nsview %p): %s\n",
			     TIC_MARKED_TEXT, bdk_window, self,
			     str ? str : "(empty)"));

  /* handle text input changes by mouse events */
  if (!BPOINTER_TO_UINT (g_object_get_data (B_OBJECT (bdk_window),
                                            TIC_IN_KEY_DOWN)))
    {
      _bdk_quartz_synthesize_null_key_event(bdk_window);
    }
}

-(void)doCommandBySelector: (SEL)aSelector
{
  BDK_NOTE (EVENTS, g_print ("doCommandBySelector\n"));
  if ([self respondsToSelector: aSelector])
    [self performSelector: aSelector];
}

/* This gets called on OS X 10.6 and upwards from interpretKeyEvents */
-(void)insertText: (id)aString replacementRange: (NSRange)replacementRange
{
  [self insertText:aString];
}

/* This gets called on OS X 10.5 from interpretKeyEvents, although 10.5
 * is supposed to support NSTextInputClient  */
-(void)insertText: (id)aString
{
  BDK_NOTE (EVENTS, g_print ("insertText\n"));
  const char *str;
  NSString *string;
  bchar *prev_str;

  if ([self hasMarkedText])
    [self unmarkText];

  if ([aString isKindOfClass: [NSAttributedString class]])
      string = [aString string];
  else
      string = aString;

  NSCharacterSet *ctrlChars = [NSCharacterSet controlCharacterSet];
  NSCharacterSet *wsnlChars = [NSCharacterSet whitespaceAndNewlineCharacterSet];
  if ([string rangeOfCharacterFromSet:ctrlChars].length &&
      [string rangeOfCharacterFromSet:wsnlChars].length == 0)
    {
      /* discard invalid text input with Chinese input methods */
      str = "";
      [self unmarkText];
      NSInputManager *currentInputManager = [NSInputManager currentInputManager];
      [currentInputManager markedTextAbandoned:self];
    }
  else
   {
      str = [string UTF8String];
   }

  prev_str = g_object_get_data (B_OBJECT (bdk_window), TIC_INSERT_TEXT);
  if (prev_str)
    g_free (prev_str);
  g_object_set_data (B_OBJECT (bdk_window), TIC_INSERT_TEXT, g_strdup (str));
  BDK_NOTE (EVENTS, g_print ("insertText: set %s (%p, nsview %p): %s\n",
			     TIC_INSERT_TEXT, bdk_window, self,
			     str ? str : "(empty)"));

  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_FILTERED));

  /* handle text input changes by mouse events */
  if (!BPOINTER_TO_UINT (g_object_get_data (B_OBJECT (bdk_window),
                                            TIC_IN_KEY_DOWN)))
    {
      _bdk_quartz_synthesize_null_key_event(bdk_window);
    }
}

-(void)deleteBackward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("deleteBackward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteForward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("deleteForward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteToBeginningOfLine: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("deleteToBeginningOfLine\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteToEndOfLine: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("deleteToEndOfLine\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteWordBackward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("deleteWordBackward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)deleteWordForward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("deleteWordForward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertBacktab: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("insertBacktab\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertNewline: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("insertNewline\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY, BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)insertTab: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("insertTab\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveBackward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveBackward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveBackwardAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveBackwardAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveDown: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveDown\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveDownAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveDownAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveForward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveForward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveForwardAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveForwardAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveLeft: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveLeft\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveLeftAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveLeftAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveRight: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveRight\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveRightAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveRightAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfDocument: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToBeginningOfDocument\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfDocumentAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToBeginningOfDocumentAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfLine: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToBeginningOfLine\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToBeginningOfLineAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToBeginningOfLineAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfDocument: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToEndOfDocument\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfDocumentAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToEndOfDocumentAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfLine: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToEndOfLine\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveToEndOfLineAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveToEndOfLineAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveUp: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveUp\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveUpAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveUpAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordBackward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordBackward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordBackwardAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordBackwardAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordForward: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordForward\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordForwardAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordForwardAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordLeft: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordLeft\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordLeftAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordLeftAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordRight: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordRight\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)moveWordRightAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("moveWordRightAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageDown: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("pageDown\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageDownAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("pageDownAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageUp: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("pageUp\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)pageUpAndModifySelection: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("pageUpAndModifySelection\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)selectAll: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("selectAll\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)selectLine: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("selectLine\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)selectWord: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("selectWord\n"));
  g_object_set_data (B_OBJECT (bdk_window), GIC_FILTER_KEY,
		     BUINT_TO_POINTER (GIC_FILTER_PASSTHRU));
}

-(void)noop: (id)sender
{
  BDK_NOTE (EVENTS, g_print ("noop\n"));
}

/* --------------------------------------------------------------- */

-(void)dealloc
{
  if (trackingRect)
    {
      [self removeTrackingRect: trackingRect];
      trackingRect = 0;
    }

  [super dealloc];
}

-(void)setBdkWindow: (BdkWindow *)window
{
  bdk_window = window;
}

-(BdkWindow *)bdkWindow
{
  return bdk_window;
}

-(NSTrackingRectTag)trackingRect
{
  return trackingRect;
}

-(BOOL)isFlipped
{
  return YES;
}

-(BOOL)isOpaque
{
  if (BDK_WINDOW_DESTROYED (bdk_window))
    return YES;

  /* A view is opaque if its BdkWindow doesn't have the RGBA colormap */
  return bdk_drawable_get_colormap (bdk_window) !=
    bdk_screen_get_rgba_colormap (_bdk_screen);
}

-(void)drawRect: (NSRect)rect
{
  BdkRectangle bdk_rect;
  BdkWindowObject *private = BDK_WINDOW_OBJECT (bdk_window);
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  const NSRect *drawn_rects;
  NSInteger count;
  int i;
  BdkRebunnyion *rebunnyion;

  if (BDK_WINDOW_DESTROYED (bdk_window))
    return;

  if (! (private->event_mask & BDK_EXPOSURE_MASK))
    return;

  if (NSEqualRects (rect, NSZeroRect))
    return;

  if (!BDK_WINDOW_IS_MAPPED (bdk_window))
    {
      /* If the window is not yet mapped, clip_rebunnyion_with_children
       * will be empty causing the usual code below to draw nothing.
       * To not see garbage on the screen, we draw an aesthetic color
       * here. The garbage would be visible if any widget enabled
       * the NSView's CALayer in order to add sublayers for custom
       * native rendering.
       */
      [NSGraphicsContext saveGraphicsState];

      [[NSColor windowBackgroundColor] setFill];
      [NSBezierPath fillRect: rect];

      [NSGraphicsContext restoreGraphicsState];

      return;
    }

  /* Clear our own bookkeeping of rebunnyions that need display */
  if (impl->needs_display_rebunnyion)
    {
      bdk_rebunnyion_destroy (impl->needs_display_rebunnyion);
      impl->needs_display_rebunnyion = NULL;
    }

  [self getRectsBeingDrawn: &drawn_rects count: &count];
  rebunnyion = bdk_rebunnyion_new ();

  for (i = 0; i < count; i++)
    {
      bdk_rect.x = drawn_rects[i].origin.x;
      bdk_rect.y = drawn_rects[i].origin.y;
      bdk_rect.width = drawn_rects[i].size.width;
      bdk_rect.height = drawn_rects[i].size.height;

      bdk_rebunnyion_union_with_rect (rebunnyion, &bdk_rect);
    }

  impl->in_paint_rect_count++;
  _bdk_window_process_updates_recurse (bdk_window, rebunnyion);
  impl->in_paint_rect_count--;

  bdk_rebunnyion_destroy (rebunnyion);

  if (needsInvalidateShadow)
    {
      [[self window] invalidateShadow];
      needsInvalidateShadow = NO;
    }
}

-(void)setNeedsInvalidateShadow: (BOOL)invalidate
{
  needsInvalidateShadow = invalidate;
}

/* For information on setting up tracking rects properly, see here:
 * http://developer.apple.com/documentation/Cocoa/Conceptual/EventOverview/EventOverview.pdf
 */
-(void)updateTrackingRect
{
  BdkWindowObject *private = BDK_WINDOW_OBJECT (bdk_window);
  BdkWindowImplQuartz *impl = BDK_WINDOW_IMPL_QUARTZ (private->impl);
  NSRect rect;

  if (!impl || !impl->toplevel)
    return;

  if (trackingRect)
    {
      [self removeTrackingRect: trackingRect];
      trackingRect = 0;
    }

  if (!impl->toplevel)
    return;

  /* Note, if we want to set assumeInside we can use:
   * NSPointInRect ([[self window] convertScreenToBase:[NSEvent mouseLocation]], rect)
   */

  rect = [self bounds];
  trackingRect = [self addTrackingRect: rect
		  owner: self
		  userData: nil
		  assumeInside: NO];
}

-(void)viewDidMoveToWindow
{
  if (![self window]) /* We are destroyed already */
    return;

  [self updateTrackingRect];
}

-(void)viewWillMoveToWindow: (NSWindow *)newWindow
{
  if (newWindow == nil && trackingRect)
    {
      [self removeTrackingRect: trackingRect];
      trackingRect = 0;
    }
}

-(void)setFrame: (NSRect)frame
{
  [super setFrame: frame];

  if ([self window])
    [self updateTrackingRect];
}

@end
