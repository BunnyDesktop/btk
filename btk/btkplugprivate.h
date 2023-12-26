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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the BTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#ifndef __BTK_PLUG_PRIVATE_H__
#define __BTK_PLUG_PRIVATE_H__

/* In btkplug.c: */
void _btk_plug_send_delete_event      (BtkWidget        *widget);
void _btk_plug_add_all_grabbed_keys   (BtkPlug          *plug);
void _btk_plug_focus_first_last       (BtkPlug          *plug,
				       BtkDirectionType  direction);
void _btk_plug_handle_modality_on     (BtkPlug          *plug);
void _btk_plug_handle_modality_off    (BtkPlug          *plug);

/* In backend-specific file: */

/*
 * _btk_plug_windowing_get_id:
 *
 * @plug: a #BtkPlug
 *
 * Returns the native window system identifier for the plug's window.
 */
BdkNativeWindow _btk_plug_windowing_get_id (BtkPlug *plug);

/*
 * _btk_plug_windowing_realize_toplevel:
 *
 * @plug_window: a #BtkPlug's #BdkWindow
 *
 * Called from BtkPlug's realize method. Should tell the corresponding
 * socket that the plug has been realized.
 */
void _btk_plug_windowing_realize_toplevel (BtkPlug *plug);

/*
 * _btk_plug_windowing_map_toplevel:
 *
 * @plug: a #BtkPlug
 *
 * Called from BtkPlug's map method. Should tell the corresponding
 * #BtkSocket that the plug has been mapped.
 */
void _btk_plug_windowing_map_toplevel (BtkPlug *plug);

/*
 * _btk_plug_windowing_map_toplevel:
 *
 * @plug: a #BtkPlug
 *
 * Called from BtkPlug's unmap method. Should tell the corresponding
 * #BtkSocket that the plug has been unmapped.
 */
void _btk_plug_windowing_unmap_toplevel (BtkPlug *plug);

/*
 * _btk_plug_windowing_set_focus:
 *
 * @plug: a #BtkPlug
 *
 * Called from BtkPlug's set_focus method. Should tell the corresponding
 * #BtkSocket to request focus.
 */
void _btk_plug_windowing_set_focus (BtkPlug *plug);

/*
 * _btk_plug_windowing_add_grabbed_key:
 *
 * @plug: a #BtkPlug
 * @accelerator_key: a key
 * @accelerator_mods: modifiers for it
 *
 * Called from BtkPlug's keys_changed method. Should tell the
 * corresponding #BtkSocket to grab the key.
 */
void _btk_plug_windowing_add_grabbed_key (BtkPlug         *plug,
					  buint            accelerator_key,
					  BdkModifierType  accelerator_mods);

/*
 * _btk_plug_windowing_remove_grabbed_key:
 *
 * @plug: a #BtkPlug
 * @accelerator_key: a key
 * @accelerator_mods: modifiers for it
 *
 * Called from BtkPlug's keys_changed method. Should tell the
 * corresponding #BtkSocket to remove the key grab.
 */
void _btk_plug_windowing_remove_grabbed_key (BtkPlug         *plug,
					     buint            accelerator_key,
					     BdkModifierType  accelerator_mods);

/*
 * _btk_plug_windowing_focus_to_parent:
 *
 * @plug: a #BtkPlug
 * @direction: a direction
 *
 * Called from BtkPlug's focus method. Should tell the corresponding
 * #BtkSocket to move the focus.
 */
void _btk_plug_windowing_focus_to_parent (BtkPlug         *plug,
					  BtkDirectionType direction);

/*
 * _btk_plug_windowing_filter_func:
 *
 * @bdk_xevent: a windowing system native event
 * @event: a pre-allocated empty BdkEvent
 * @data: the #BtkPlug
 *
 * Event filter function installed on plug windows.
 */
BdkFilterReturn _btk_plug_windowing_filter_func (BdkXEvent *bdk_xevent,
						 BdkEvent  *event,
						 bpointer   data);

#endif /* __BTK_PLUG_PRIVATE_H__ */
