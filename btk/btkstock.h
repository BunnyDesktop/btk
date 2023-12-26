/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __BTK_STOCK_H__
#define __BTK_STOCK_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btktypeutils.h> /* for BtkTranslateFunc */

B_BEGIN_DECLS

typedef struct _BtkStockItem BtkStockItem;

struct _BtkStockItem
{
  gchar *stock_id;
  gchar *label;
  BdkModifierType modifier;
  guint keyval;
  gchar *translation_domain;
};

void     btk_stock_add        (const BtkStockItem  *items,
                               guint                n_items);
void     btk_stock_add_static (const BtkStockItem  *items,
                               guint                n_items);
gboolean btk_stock_lookup     (const gchar         *stock_id,
                               BtkStockItem        *item);

/* Should free the list (and free each string in it also).
 * This function is only useful for GUI builders and such.
 */
GSList*  btk_stock_list_ids  (void);

BtkStockItem *btk_stock_item_copy (const BtkStockItem *item);
void          btk_stock_item_free (BtkStockItem       *item);

void          btk_stock_set_translate_func (const gchar      *domain,
					    BtkTranslateFunc  func,
					    gpointer          data,
					    GDestroyNotify    notify);

/* Stock IDs (not all are stock items; some are images only) */
/**
 * BTK_STOCK_ABOUT:
 *
 * The "About" item.
 * <inlinegraphic fileref="help-about.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_ABOUT            "btk-about"

/**
 * BTK_STOCK_ADD:
 *
 * The "Add" item.
 * <inlinegraphic fileref="list-add.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_ADD              "btk-add"

/**
 * BTK_STOCK_APPLY:
 *
 * The "Apply" item.
 * <inlinegraphic fileref="btk-apply.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_APPLY            "btk-apply"

/**
 * BTK_STOCK_BOLD:
 *
 * The "Bold" item.
 * <inlinegraphic fileref="format-text-bold.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_BOLD             "btk-bold"

/**
 * BTK_STOCK_CANCEL:
 *
 * The "Cancel" item.
 * <inlinegraphic fileref="btk-cancel.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_CANCEL           "btk-cancel"

/**
 * BTK_STOCK_CAPS_LOCK_WARNING:
 *
 * The "Caps Lock Warning" icon.
 * <inlinegraphic fileref="btk-caps-lock-warning.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.16
 */
#define BTK_STOCK_CAPS_LOCK_WARNING "btk-caps-lock-warning"

/**
 * BTK_STOCK_CDROM:
 *
 * The "CD-Rom" item.
 * <inlinegraphic fileref="media-optical.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_CDROM            "btk-cdrom"

/**
 * BTK_STOCK_CLEAR:
 *
 * The "Clear" item.
 * <inlinegraphic fileref="edit-clear.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_CLEAR            "btk-clear"

/**
 * BTK_STOCK_CLOSE:
 *
 * The "Close" item.
 * <inlinegraphic fileref="window-close.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_CLOSE            "btk-close"

/**
 * BTK_STOCK_COLOR_PICKER:
 *
 * The "Color Picker" item.
 * <inlinegraphic fileref="btk-color-picker.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.2
 */
#define BTK_STOCK_COLOR_PICKER     "btk-color-picker"

/**
 * BTK_STOCK_CONNECT:
 *
 * The "Connect" icon.
 * <inlinegraphic fileref="btk-connect.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_CONNECT          "btk-connect"

/**
 * BTK_STOCK_CONVERT:
 *
 * The "Convert" item.
 * <inlinegraphic fileref="btk-convert.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_CONVERT          "btk-convert"

/**
 * BTK_STOCK_COPY:
 *
 * The "Copy" item.
 * <inlinegraphic fileref="edit-copy.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_COPY             "btk-copy"

/**
 * BTK_STOCK_CUT:
 *
 * The "Cut" item.
 * <inlinegraphic fileref="edit-cut.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_CUT              "btk-cut"

/**
 * BTK_STOCK_DELETE:
 *
 * The "Delete" item.
 * <inlinegraphic fileref="edit-delete.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DELETE           "btk-delete"

/**
 * BTK_STOCK_DIALOG_AUTHENTICATION:
 *
 * The "Authentication" item.
 * <inlinegraphic fileref="dialog-password.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.4
 */
#define BTK_STOCK_DIALOG_AUTHENTICATION "btk-dialog-authentication"

/**
 * BTK_STOCK_DIALOG_INFO:
 *
 * The "Information" item.
 * <inlinegraphic fileref="dialog-information.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DIALOG_INFO      "btk-dialog-info"

/**
 * BTK_STOCK_DIALOG_WARNING:
 *
 * The "Warning" item.
 * <inlinegraphic fileref="dialog-warning.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DIALOG_WARNING   "btk-dialog-warning"

/**
 * BTK_STOCK_DIALOG_ERROR:
 *
 * The "Error" item.
 * <inlinegraphic fileref="dialog-error.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DIALOG_ERROR     "btk-dialog-error"

/**
 * BTK_STOCK_DIALOG_QUESTION:
 *
 * The "Question" item.
 * <inlinegraphic fileref="dialog-question.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DIALOG_QUESTION  "btk-dialog-question"

/**
 * BTK_STOCK_DIRECTORY:
 *
 * The "Directory" icon.
 * <inlinegraphic fileref="folder.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_DIRECTORY        "btk-directory"

/**
 * BTK_STOCK_DISCARD:
 *
 * The "Discard" item.
 *
 * Since: 2.12
 */
#define BTK_STOCK_DISCARD          "btk-discard"

/**
 * BTK_STOCK_DISCONNECT:
 *
 * The "Disconnect" icon.
 * <inlinegraphic fileref="btk-disconnect.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_DISCONNECT       "btk-disconnect"

/**
 * BTK_STOCK_DND:
 *
 * The "Drag-And-Drop" icon.
 * <inlinegraphic fileref="btk-dnd.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DND              "btk-dnd"

/**
 * BTK_STOCK_DND_MULTIPLE:
 *
 * The "Drag-And-Drop multiple" icon.
 * <inlinegraphic fileref="btk-dnd-multiple.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_DND_MULTIPLE     "btk-dnd-multiple"

/**
 * BTK_STOCK_EDIT:
 *
 * The "Edit" item.
 * <inlinegraphic fileref="btk-edit.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_EDIT             "btk-edit"

/**
 * BTK_STOCK_EXECUTE:
 *
 * The "Execute" item.
 * <inlinegraphic fileref="system-run.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_EXECUTE          "btk-execute"

/**
 * BTK_STOCK_FILE:
 *
 * The "File" icon.
 * <inlinegraphic fileref="document-x-generic.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_FILE             "btk-file"

/**
 * BTK_STOCK_FIND:
 *
 * The "Find" item.
 * <inlinegraphic fileref="edit-find.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_FIND             "btk-find"

/**
 * BTK_STOCK_FIND_AND_REPLACE:
 *
 * The "Find and Replace" item.
 * <inlinegraphic fileref="edit-find-replace.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_FIND_AND_REPLACE "btk-find-and-replace"

/**
 * BTK_STOCK_FLOPPY:
 *
 * The "Floppy" item.
 * <inlinegraphic fileref="media-floppy.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_FLOPPY           "btk-floppy"

/**
 * BTK_STOCK_FULLSCREEN:
 *
 * The "Fullscreen" item.
 * <inlinegraphic fileref="view-fullscreen.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.8
 */
#define BTK_STOCK_FULLSCREEN       "btk-fullscreen"

/**
 * BTK_STOCK_GOTO_BOTTOM:
 *
 * The "Bottom" item.
 * <inlinegraphic fileref="go-bottom.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GOTO_BOTTOM      "btk-goto-bottom"

/**
 * BTK_STOCK_GOTO_FIRST:
 *
 * The "First" item.
 * <inlinegraphic fileref="go-first-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="go-first-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GOTO_FIRST       "btk-goto-first"

/**
 * BTK_STOCK_GOTO_LAST:
 *
 * The "Last" item.
 * <inlinegraphic fileref="go-last-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="go-last-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GOTO_LAST        "btk-goto-last"

/**
 * BTK_STOCK_GOTO_TOP:
 *
 * The "Top" item.
 * <inlinegraphic fileref="go-top.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GOTO_TOP         "btk-goto-top"

/**
 * BTK_STOCK_GO_BACK:
 *
 * The "Back" item.
 * <inlinegraphic fileref="go-previous-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="go-previous-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GO_BACK          "btk-go-back"

/**
 * BTK_STOCK_GO_DOWN:
 *
 * The "Down" item.
 * <inlinegraphic fileref="go-down.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GO_DOWN          "btk-go-down"

/**
 * BTK_STOCK_GO_FORWARD:
 *
 * The "Forward" item.
 * <inlinegraphic fileref="go-next-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="go-next-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GO_FORWARD       "btk-go-forward"

/**
 * BTK_STOCK_GO_UP:
 *
 * The "Up" item.
 * <inlinegraphic fileref="go-up.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_GO_UP            "btk-go-up"

/**
 * BTK_STOCK_HARDDISK:
 *
 * The "Harddisk" item.
 * <inlinegraphic fileref="drive-harddisk.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.4
 */
#define BTK_STOCK_HARDDISK         "btk-harddisk"

/**
 * BTK_STOCK_HELP:
 *
 * The "Help" item.
 * <inlinegraphic fileref="help-contents.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_HELP             "btk-help"

/**
 * BTK_STOCK_HOME:
 *
 * The "Home" item.
 * <inlinegraphic fileref="go-home.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_HOME             "btk-home"

/**
 * BTK_STOCK_INDEX:
 *
 * The "Index" item.
 * <inlinegraphic fileref="btk-index.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_INDEX            "btk-index"

/**
 * BTK_STOCK_INDENT:
 *
 * The "Indent" item.
 * <inlinegraphic fileref="btk-indent-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="btk-indent-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.4
 */
#define BTK_STOCK_INDENT           "btk-indent"

/**
 * BTK_STOCK_INFO:
 *
 * The "Info" item.
 * <inlinegraphic fileref="dialog-information.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.8
 */
#define BTK_STOCK_INFO             "btk-info"

/**
 * BTK_STOCK_ITALIC:
 *
 * The "Italic" item.
 * <inlinegraphic fileref="format-text-italic.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_ITALIC           "btk-italic"

/**
 * BTK_STOCK_JUMP_TO:
 *
 * The "Jump to" item.
 * <inlinegraphic fileref="go-jump-ltr.png" format="PNG"></inlinegraphic>
 * RTL-variant
 * <inlinegraphic fileref="go-jump-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_JUMP_TO          "btk-jump-to"

/**
 * BTK_STOCK_JUSTIFY_CENTER:
 *
 * The "Center" item.
 * <inlinegraphic fileref="format-justify-center.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_JUSTIFY_CENTER   "btk-justify-center"

/**
 * BTK_STOCK_JUSTIFY_FILL:
 *
 * The "Fill" item.
 * <inlinegraphic fileref="format-justify-fill.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_JUSTIFY_FILL     "btk-justify-fill"

/**
 * BTK_STOCK_JUSTIFY_LEFT:
 *
 * The "Left" item.
 * <inlinegraphic fileref="format-justify-left.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_JUSTIFY_LEFT     "btk-justify-left"

/**
 * BTK_STOCK_JUSTIFY_RIGHT:
 *
 * The "Right" item.
 * <inlinegraphic fileref="format-justify-right.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_JUSTIFY_RIGHT    "btk-justify-right"

/**
 * BTK_STOCK_LEAVE_FULLSCREEN:
 *
 * The "Leave Fullscreen" item.
 * <inlinegraphic fileref="view-restore.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.8
 */
#define BTK_STOCK_LEAVE_FULLSCREEN "btk-leave-fullscreen"

/**
 * BTK_STOCK_MISSING_IMAGE:
 *
 * The "Missing image" icon.
 * <inlinegraphic fileref="image-missing.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_MISSING_IMAGE    "btk-missing-image"

/**
 * BTK_STOCK_MEDIA_FORWARD:
 *
 * The "Media Forward" item.
 * <inlinegraphic fileref="media-seek-forward-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="media-seek-forward-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_FORWARD    "btk-media-forward"

/**
 * BTK_STOCK_MEDIA_NEXT:
 *
 * The "Media Next" item.
 * <inlinegraphic fileref="media-skip-forward-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="media-skip-forward-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_NEXT       "btk-media-next"

/**
 * BTK_STOCK_MEDIA_PAUSE:
 *
 * The "Media Pause" item.
 * <inlinegraphic fileref="media-playback-pause.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_PAUSE      "btk-media-pause"

/**
 * BTK_STOCK_MEDIA_PLAY:
 *
 * The "Media Play" item.
 * <inlinegraphic fileref="media-playback-start-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="media-playback-start-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_PLAY       "btk-media-play"

/**
 * BTK_STOCK_MEDIA_PREVIOUS:
 *
 * The "Media Previous" item.
 * <inlinegraphic fileref="media-skip-backward-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="media-skip-backward-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_PREVIOUS   "btk-media-previous"

/**
 * BTK_STOCK_MEDIA_RECORD:
 *
 * The "Media Record" item.
 * <inlinegraphic fileref="media-record.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_RECORD     "btk-media-record"

/**
 * BTK_STOCK_MEDIA_REWIND:
 *
 * The "Media Rewind" item.
 * <inlinegraphic fileref="media-seek-backward-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="media-seek-backward-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_REWIND     "btk-media-rewind"

/**
 * BTK_STOCK_MEDIA_STOP:
 *
 * The "Media Stop" item.
 * <inlinegraphic fileref="media-playback-stop.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.6
 */
#define BTK_STOCK_MEDIA_STOP       "btk-media-stop"

/**
 * BTK_STOCK_NETWORK:
 *
 * The "Network" item.
 * <inlinegraphic fileref="network-idle.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.4
 */
#define BTK_STOCK_NETWORK          "btk-network"

/**
 * BTK_STOCK_NEW:
 *
 * The "New" item.
 * <inlinegraphic fileref="document-new.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_NEW              "btk-new"

/**
 * BTK_STOCK_NO:
 *
 * The "No" item.
 * <inlinegraphic fileref="btk-no.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_NO               "btk-no"

/**
 * BTK_STOCK_OK:
 *
 * The "OK" item.
 * <inlinegraphic fileref="btk-ok.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_OK               "btk-ok"

/**
 * BTK_STOCK_OPEN:
 *
 * The "Open" item.
 * <inlinegraphic fileref="document-open.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_OPEN             "btk-open"

/**
 * BTK_STOCK_ORIENTATION_PORTRAIT:
 *
 * The "Portrait Orientation" item.
 * <inlinegraphic fileref="btk-orientation-portrait.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.10
 */
#define BTK_STOCK_ORIENTATION_PORTRAIT "btk-orientation-portrait"

/**
 * BTK_STOCK_ORIENTATION_LANDSCAPE:
 *
 * The "Landscape Orientation" item.
 * <inlinegraphic fileref="btk-orientation-landscape.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.10
 */
#define BTK_STOCK_ORIENTATION_LANDSCAPE "btk-orientation-landscape"

/**
 * BTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE:
 *
 * The "Reverse Landscape Orientation" item.
 * <inlinegraphic fileref="btk-orientation-reverse-landscape.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.10
 */
#define BTK_STOCK_ORIENTATION_REVERSE_LANDSCAPE "btk-orientation-reverse-landscape"

/**
 * BTK_STOCK_ORIENTATION_REVERSE_PORTRAIT:
 *
 * The "Reverse Portrait Orientation" item.
 * <inlinegraphic fileref="btk-orientation-reverse-portrait.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.10
 */
#define BTK_STOCK_ORIENTATION_REVERSE_PORTRAIT "btk-orientation-reverse-portrait"

/**
 * BTK_STOCK_PAGE_SETUP:
 *
 * The "Page Setup" item.
 * <inlinegraphic fileref="btk-page-setup.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.14
 */
#define BTK_STOCK_PAGE_SETUP       "btk-page-setup"

/**
 * BTK_STOCK_PASTE:
 *
 * The "Paste" item.
 * <inlinegraphic fileref="edit-paste.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_PASTE            "btk-paste"

/**
 * BTK_STOCK_PREFERENCES:
 *
 * The "Preferences" item.
 * <inlinegraphic fileref="btk-preferences.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_PREFERENCES      "btk-preferences"

/**
 * BTK_STOCK_PRINT:
 *
 * The "Print" item.
 * <inlinegraphic fileref="document-print.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_PRINT            "btk-print"

/**
 * BTK_STOCK_PRINT_ERROR:
 *
 * The "Print Error" icon.
 * <inlinegraphic fileref="printer-error.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.14
 */
#define BTK_STOCK_PRINT_ERROR      "btk-print-error"

/**
 * BTK_STOCK_PRINT_PAUSED:
 *
 * The "Print Paused" icon.
 * <inlinegraphic fileref="printer-paused.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.14
 */
#define BTK_STOCK_PRINT_PAUSED     "btk-print-paused"

/**
 * BTK_STOCK_PRINT_PREVIEW:
 *
 * The "Print Preview" item.
 * <inlinegraphic fileref="document-print-preview.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_PRINT_PREVIEW    "btk-print-preview"

/**
 * BTK_STOCK_PRINT_REPORT:
 *
 * The "Print Report" icon.
 * <inlinegraphic fileref="printer-info.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.14
 */
#define BTK_STOCK_PRINT_REPORT     "btk-print-report"


/**
 * BTK_STOCK_PRINT_WARNING:
 *
 * The "Print Warning" icon.
 * <inlinegraphic fileref="printer-warning.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.14
 */
#define BTK_STOCK_PRINT_WARNING    "btk-print-warning"

/**
 * BTK_STOCK_PROPERTIES:
 *
 * The "Properties" item.
 * <inlinegraphic fileref="document-properties.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_PROPERTIES       "btk-properties"

/**
 * BTK_STOCK_QUIT:
 *
 * The "Quit" item.
 * <inlinegraphic fileref="application-exit.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_QUIT             "btk-quit"

/**
 * BTK_STOCK_REDO:
 *
 * The "Redo" item.
 * <inlinegraphic fileref="edit-redo-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="edit-redo-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_REDO             "btk-redo"

/**
 * BTK_STOCK_REFRESH:
 *
 * The "Refresh" item.
 * <inlinegraphic fileref="view-refresh.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_REFRESH          "btk-refresh"

/**
 * BTK_STOCK_REMOVE:
 *
 * The "Remove" item.
 * <inlinegraphic fileref="list-remove.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_REMOVE           "btk-remove"

/**
 * BTK_STOCK_REVERT_TO_SAVED:
 *
 * The "Revert" item.
 * <inlinegraphic fileref="document-revert-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="document-revert-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_REVERT_TO_SAVED  "btk-revert-to-saved"

/**
 * BTK_STOCK_SAVE:
 *
 * The "Save" item.
 * <inlinegraphic fileref="document-save.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SAVE             "btk-save"

/**
 * BTK_STOCK_SAVE_AS:
 *
 * The "Save As" item.
 * <inlinegraphic fileref="document-save-as.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SAVE_AS          "btk-save-as"

/**
 * BTK_STOCK_SELECT_ALL:
 *
 * The "Select All" item.
 * <inlinegraphic fileref="edit-select-all.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.10
 */
#define BTK_STOCK_SELECT_ALL       "btk-select-all"

/**
 * BTK_STOCK_SELECT_COLOR:
 *
 * The "Color" item.
 * <inlinegraphic fileref="btk-select-color.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SELECT_COLOR     "btk-select-color"

/**
 * BTK_STOCK_SELECT_FONT:
 *
 * The "Font" item.
 * <inlinegraphic fileref="btk-font.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SELECT_FONT      "btk-select-font"

/**
 * BTK_STOCK_SORT_ASCENDING:
 *
 * The "Ascending" item.
 * <inlinegraphic fileref="view-sort-ascending.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SORT_ASCENDING   "btk-sort-ascending"

/**
 * BTK_STOCK_SORT_DESCENDING:
 *
 * The "Descending" item.
 * <inlinegraphic fileref="view-sort-descending.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SORT_DESCENDING  "btk-sort-descending"

/**
 * BTK_STOCK_SPELL_CHECK:
 *
 * The "Spell Check" item.
 * <inlinegraphic fileref="tools-check-spelling.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_SPELL_CHECK      "btk-spell-check"

/**
 * BTK_STOCK_STOP:
 *
 * The "Stop" item.
 * <inlinegraphic fileref="process-stop.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_STOP             "btk-stop"

/**
 * BTK_STOCK_STRIKETHROUGH:
 *
 * The "Strikethrough" item.
 * <inlinegraphic fileref="format-text-strikethrough.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_STRIKETHROUGH    "btk-strikethrough"

/**
 * BTK_STOCK_UNDELETE:
 *
 * The "Undelete" item.
 * <inlinegraphic fileref="btk-undelete-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="btk-undelete-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_UNDELETE         "btk-undelete"

/**
 * BTK_STOCK_UNDERLINE:
 *
 * The "Underline" item.
 * <inlinegraphic fileref="format-text-underline.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_UNDERLINE        "btk-underline"

/**
 * BTK_STOCK_UNDO:
 *
 * The "Undo" item.
 * <inlinegraphic fileref="edit-undo-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="edit-undo-rtl.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_UNDO             "btk-undo"

/**
 * BTK_STOCK_UNINDENT:
 *
 * The "Unindent" item.
 * <inlinegraphic fileref="format-indent-less-ltr.png" format="PNG"></inlinegraphic>
 * RTL variant
 * <inlinegraphic fileref="format-indent-less-rtl.png" format="PNG"></inlinegraphic>
 *
 * Since: 2.4
 */
#define BTK_STOCK_UNINDENT         "btk-unindent"

/**
 * BTK_STOCK_YES:
 *
 * The "Yes" item.
 * <inlinegraphic fileref="btk-yes.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_YES              "btk-yes"

/**
 * BTK_STOCK_ZOOM_100:
 *
 * The "Zoom 100%" item.
 * <inlinegraphic fileref="zoom-original.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_ZOOM_100         "btk-zoom-100"

/**
 * BTK_STOCK_ZOOM_FIT:
 *
 * The "Zoom to Fit" item.
 * <inlinegraphic fileref="zoom-fit-best.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_ZOOM_FIT         "btk-zoom-fit"

/**
 * BTK_STOCK_ZOOM_IN:
 *
 * The "Zoom In" item.
 * <inlinegraphic fileref="zoom-in.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_ZOOM_IN          "btk-zoom-in"

/**
 * BTK_STOCK_ZOOM_OUT:
 *
 * The "Zoom Out" item.
 * <inlinegraphic fileref="zoom-out.png" format="PNG"></inlinegraphic>
 */
#define BTK_STOCK_ZOOM_OUT         "btk-zoom-out"

B_END_DECLS

#endif /* __BTK_STOCK_H__ */
