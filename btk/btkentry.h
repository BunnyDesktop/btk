/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2008 Cody Russell
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __BTK_ENTRY_H__
#define __BTK_ENTRY_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkeditable.h>
#include <btk/btkimcontext.h>
#include <btk/btkmenu.h>
#include <btk/btkentrybuffer.h>
#include <btk/btkentrycompletion.h>
#include <btk/btkimage.h>
#include <btk/btkselection.h>


B_BEGIN_DECLS

#define BTK_TYPE_ENTRY                  (btk_entry_get_type ())
#define BTK_ENTRY(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ENTRY, BtkEntry))
#define BTK_ENTRY_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ENTRY, BtkEntryClass))
#define BTK_IS_ENTRY(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ENTRY))
#define BTK_IS_ENTRY_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ENTRY))
#define BTK_ENTRY_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ENTRY, BtkEntryClass))

typedef enum
{
  BTK_ENTRY_ICON_PRIMARY,
  BTK_ENTRY_ICON_SECONDARY
} BtkEntryIconPosition;

typedef struct _BtkEntry       BtkEntry;
typedef struct _BtkEntryClass  BtkEntryClass;

struct _BtkEntry
{
  BtkWidget  widget;

  bchar       *GSEAL (text);                        /* COMPAT: Deprecated, not used. Remove in BTK+ 3.x */

  buint        GSEAL (editable) : 1;
  buint        GSEAL (visible)  : 1;
  buint        GSEAL (overwrite_mode) : 1;
  buint        GSEAL (in_drag) : 1;	            /* FIXME: Should be private?
                                                       Dragging within the selection */

  buint16      GSEAL (text_length);                 /* COMPAT: Deprecated, not used. Remove in BTK+ 3.x */
  buint16      GSEAL (text_max_length);             /* COMPAT: Deprecated, not used. Remove in BTK+ 3.x */

  /*< private >*/
  BdkWindow    *GSEAL (text_area);
  BtkIMContext *GSEAL (im_context);
  BtkWidget    *GSEAL (popup_menu);

  bint          GSEAL (current_pos);
  bint          GSEAL (selection_bound);

  BangoLayout  *GSEAL (cached_layout);

  buint         GSEAL (cache_includes_preedit) : 1;
  buint         GSEAL (need_im_reset)          : 1;
  buint         GSEAL (has_frame)              : 1;
  buint         GSEAL (activates_default)      : 1;
  buint         GSEAL (cursor_visible)         : 1;
  buint         GSEAL (in_click)               : 1; /* Flag so we don't select all when clicking in entry to focus in */
  buint         GSEAL (is_cell_renderer)       : 1;
  buint         GSEAL (editing_canceled)       : 1; /* Only used by BtkCellRendererText */ 
  buint         GSEAL (mouse_cursor_obscured)  : 1;
  buint         GSEAL (select_words)           : 1;
  buint         GSEAL (select_lines)           : 1;
  buint         GSEAL (resolved_dir)           : 4; /* BangoDirection */
  buint         GSEAL (truncate_multiline)     : 1;

  buint         GSEAL (button);
  buint         GSEAL (blink_timeout);
  buint         GSEAL (recompute_idle);
  bint          GSEAL (scroll_offset);
  bint          GSEAL (ascent);	                    /* font ascent in bango units  */
  bint          GSEAL (descent);	            /* font descent in bango units */

  buint16       GSEAL (x_text_size);	            /* allocated size, in bytes */
  buint16       GSEAL (x_n_bytes);	            /* length in use, in bytes */

  buint16       GSEAL (preedit_length);	            /* length of preedit string, in bytes */
  buint16       GSEAL (preedit_cursor);	            /* offset of cursor within preedit string, in chars */

  bint          GSEAL (dnd_position);		    /* In chars, -1 == no DND cursor */

  bint          GSEAL (drag_start_x);
  bint          GSEAL (drag_start_y);

  gunichar      GSEAL (invisible_char);

  bint          GSEAL (width_chars);
};

struct _BtkEntryClass
{
  BtkWidgetClass parent_class;

  /* Hook to customize right-click popup */
  void (* populate_popup)   (BtkEntry       *entry,
                             BtkMenu        *menu);

  /* Action signals
   */
  void (* activate)           (BtkEntry             *entry);
  void (* move_cursor)        (BtkEntry             *entry,
			       BtkMovementStep       step,
			       bint                  count,
			       bboolean              extend_selection);
  void (* insert_at_cursor)   (BtkEntry             *entry,
			       const bchar          *str);
  void (* delete_from_cursor) (BtkEntry             *entry,
			       BtkDeleteType         type,
			       bint                  count);
  void (* backspace)          (BtkEntry             *entry);
  void (* cut_clipboard)      (BtkEntry             *entry);
  void (* copy_clipboard)     (BtkEntry             *entry);
  void (* paste_clipboard)    (BtkEntry             *entry);
  void (* toggle_overwrite)   (BtkEntry             *entry);

  /* hook to add other objects beside the entry (like in BtkSpinButton) */
  void (* get_text_area_size) (BtkEntry       *entry,
			       bint           *x,
			       bint           *y,
			       bint           *width,
			       bint           *height);

  /* Padding for future expansion */
  void (*_btk_reserved1)      (void);
  void (*_btk_reserved2)      (void);
};

GType      btk_entry_get_type       		(void) B_GNUC_CONST;
BtkWidget* btk_entry_new            		(void);
BtkWidget* btk_entry_new_with_buffer            (BtkEntryBuffer *buffer);

BtkEntryBuffer* btk_entry_get_buffer            (BtkEntry       *entry);
void       btk_entry_set_buffer                 (BtkEntry       *entry,
                                                 BtkEntryBuffer *buffer);

BdkWindow *btk_entry_get_text_window            (BtkEntry      *entry);

void       btk_entry_set_visibility 		(BtkEntry      *entry,
						 bboolean       visible);
bboolean   btk_entry_get_visibility             (BtkEntry      *entry);

void       btk_entry_set_invisible_char         (BtkEntry      *entry,
                                                 gunichar       ch);
gunichar   btk_entry_get_invisible_char         (BtkEntry      *entry);
void       btk_entry_unset_invisible_char       (BtkEntry      *entry);

void       btk_entry_set_has_frame              (BtkEntry      *entry,
                                                 bboolean       setting);
bboolean   btk_entry_get_has_frame              (BtkEntry      *entry);

void       btk_entry_set_inner_border                (BtkEntry        *entry,
                                                      const BtkBorder *border);
const BtkBorder* btk_entry_get_inner_border          (BtkEntry        *entry);

void       btk_entry_set_overwrite_mode         (BtkEntry      *entry,
                                                 bboolean       overwrite);
bboolean   btk_entry_get_overwrite_mode         (BtkEntry      *entry);

/* text is truncated if needed */
void       btk_entry_set_max_length 		(BtkEntry      *entry,
						 bint           max);
bint       btk_entry_get_max_length             (BtkEntry      *entry);
buint16    btk_entry_get_text_length            (BtkEntry      *entry);

void       btk_entry_set_activates_default      (BtkEntry      *entry,
                                                 bboolean       setting);
bboolean   btk_entry_get_activates_default      (BtkEntry      *entry);

void       btk_entry_set_width_chars            (BtkEntry      *entry,
                                                 bint           n_chars);
bint       btk_entry_get_width_chars            (BtkEntry      *entry);

/* Somewhat more convenient than the BtkEditable generic functions
 */
void       btk_entry_set_text                   (BtkEntry      *entry,
                                                 const bchar   *text);
/* returns a reference to the text */
const bchar* btk_entry_get_text                 (BtkEntry      *entry);

BangoLayout* btk_entry_get_layout               (BtkEntry      *entry);
void         btk_entry_get_layout_offsets       (BtkEntry      *entry,
                                                 bint          *x,
                                                 bint          *y);
void       btk_entry_set_alignment              (BtkEntry      *entry,
                                                 bfloat         xalign);
bfloat     btk_entry_get_alignment              (BtkEntry      *entry);

void                btk_entry_set_completion (BtkEntry           *entry,
                                              BtkEntryCompletion *completion);
BtkEntryCompletion *btk_entry_get_completion (BtkEntry           *entry);

bint       btk_entry_layout_index_to_text_index (BtkEntry      *entry,
                                                 bint           layout_index);
bint       btk_entry_text_index_to_layout_index (BtkEntry      *entry,
                                                 bint           text_index);

/* For scrolling cursor appropriately
 */
void           btk_entry_set_cursor_hadjustment (BtkEntry      *entry,
                                                 BtkAdjustment *adjustment);
BtkAdjustment* btk_entry_get_cursor_hadjustment (BtkEntry      *entry);

/* Progress API
 */
void           btk_entry_set_progress_fraction   (BtkEntry     *entry,
                                                  bdouble       fraction);
bdouble        btk_entry_get_progress_fraction   (BtkEntry     *entry);

void           btk_entry_set_progress_pulse_step (BtkEntry     *entry,
                                                  bdouble       fraction);
bdouble        btk_entry_get_progress_pulse_step (BtkEntry     *entry);

void           btk_entry_progress_pulse          (BtkEntry     *entry);

/* Setting and managing icons
 */
void           btk_entry_set_icon_from_pixbuf            (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  BdkPixbuf            *pixbuf);
void           btk_entry_set_icon_from_stock             (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  const bchar          *stock_id);
void           btk_entry_set_icon_from_icon_name         (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  const bchar          *icon_name);
void           btk_entry_set_icon_from_gicon             (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  GIcon                *icon);
BtkImageType btk_entry_get_icon_storage_type             (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
BdkPixbuf*   btk_entry_get_icon_pixbuf                   (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
const bchar* btk_entry_get_icon_stock                    (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
const bchar* btk_entry_get_icon_name                     (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
GIcon*       btk_entry_get_icon_gicon                    (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
void         btk_entry_set_icon_activatable              (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  bboolean              activatable);
bboolean     btk_entry_get_icon_activatable              (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
void         btk_entry_set_icon_sensitive                (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  bboolean              sensitive);
bboolean     btk_entry_get_icon_sensitive                (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos);
bint         btk_entry_get_icon_at_pos                   (BtkEntry             *entry,
							  bint                  x,
							  bint                  y);
void         btk_entry_set_icon_tooltip_text             (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  const bchar          *tooltip);
bchar *      btk_entry_get_icon_tooltip_text             (BtkEntry             *entry,
                                                          BtkEntryIconPosition  icon_pos);
void         btk_entry_set_icon_tooltip_markup           (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  const bchar          *tooltip);
bchar *      btk_entry_get_icon_tooltip_markup           (BtkEntry             *entry,
                                                          BtkEntryIconPosition  icon_pos);
void         btk_entry_set_icon_drag_source              (BtkEntry             *entry,
							  BtkEntryIconPosition  icon_pos,
							  BtkTargetList        *target_list,
							  BdkDragAction         actions);
bint         btk_entry_get_current_icon_drag_source      (BtkEntry             *entry);

BdkWindow  * btk_entry_get_icon_window                   (BtkEntry             *entry,
                                                          BtkEntryIconPosition  icon_pos);

bboolean    btk_entry_im_context_filter_keypress         (BtkEntry             *entry,
                                                          BdkEventKey          *event);
void        btk_entry_reset_im_context                   (BtkEntry             *entry);


/* Deprecated compatibility functions
 */

#ifndef BTK_DISABLE_DEPRECATED
BtkWidget* btk_entry_new_with_max_length	(bint           max);
void       btk_entry_append_text    		(BtkEntry      *entry,
						 const bchar   *text);
void       btk_entry_prepend_text   		(BtkEntry      *entry,
						 const bchar   *text);
void       btk_entry_set_position   		(BtkEntry      *entry,
						 bint           position);
void       btk_entry_select_rebunnyion  		(BtkEntry      *entry,
						 bint           start,
						 bint           end);
void       btk_entry_set_editable   		(BtkEntry      *entry,
						 bboolean       editable);
#endif /* BTK_DISABLE_DEPRECATED */

B_END_DECLS

#endif /* __BTK_ENTRY_H__ */
