/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkFontSelection widget for Btk+, by Damon Chaplin, May 1998.
 * Based on the BunnyFontSelector widget, by Elliot Lee, but major changes.
 * The BunnyFontSelector was derived from app/text_tool.c in the GIMP.
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

#ifndef __BTK_FONTSEL_H__
#define __BTK_FONTSEL_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdialog.h>
#include <btk/btkvbox.h>


B_BEGIN_DECLS

#define BTK_TYPE_FONT_SELECTION              (btk_font_selection_get_type ())
#define BTK_FONT_SELECTION(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FONT_SELECTION, BtkFontSelection))
#define BTK_FONT_SELECTION_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FONT_SELECTION, BtkFontSelectionClass))
#define BTK_IS_FONT_SELECTION(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FONT_SELECTION))
#define BTK_IS_FONT_SELECTION_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FONT_SELECTION))
#define BTK_FONT_SELECTION_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FONT_SELECTION, BtkFontSelectionClass))


#define BTK_TYPE_FONT_SELECTION_DIALOG              (btk_font_selection_dialog_get_type ())
#define BTK_FONT_SELECTION_DIALOG(obj)              (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FONT_SELECTION_DIALOG, BtkFontSelectionDialog))
#define BTK_FONT_SELECTION_DIALOG_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_FONT_SELECTION_DIALOG, BtkFontSelectionDialogClass))
#define BTK_IS_FONT_SELECTION_DIALOG(obj)           (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FONT_SELECTION_DIALOG))
#define BTK_IS_FONT_SELECTION_DIALOG_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_FONT_SELECTION_DIALOG))
#define BTK_FONT_SELECTION_DIALOG_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_FONT_SELECTION_DIALOG, BtkFontSelectionDialogClass))


typedef struct _BtkFontSelection	     BtkFontSelection;
typedef struct _BtkFontSelectionClass	     BtkFontSelectionClass;

typedef struct _BtkFontSelectionDialog	     BtkFontSelectionDialog;
typedef struct _BtkFontSelectionDialogClass  BtkFontSelectionDialogClass;

struct _BtkFontSelection
{
  BtkVBox parent_instance;
  
  BtkWidget *GSEAL (font_entry);        /* Used _get_family_entry() for consistency, -mr */
  BtkWidget *GSEAL (family_list);
  BtkWidget *GSEAL (font_style_entry);  /* Used _get_face_entry() for consistency, -mr */
  BtkWidget *GSEAL (face_list);
  BtkWidget *GSEAL (size_entry);
  BtkWidget *GSEAL (size_list);
  BtkWidget *GSEAL (pixels_button);     /* Unused, -mr */
  BtkWidget *GSEAL (points_button);     /* Unused, -mr */
  BtkWidget *GSEAL (filter_button);     /* Unused, -mr */
  BtkWidget *GSEAL (preview_entry);

  BangoFontFamily *GSEAL (family);	/* Current family */
  BangoFontFace *GSEAL (face);		/* Current face */
  
  gint GSEAL (size);
  
  BdkFont *GSEAL (font);		/* Cache for bdk_font_selection_get_font, so we can preserve
                                         * refcounting behavior
                                         */
};

struct _BtkFontSelectionClass
{
  BtkVBoxClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

struct _BtkFontSelectionDialog
{
  BtkDialog parent_instance;

  /*< private >*/
  BtkWidget *GSEAL (fontsel);

  BtkWidget *GSEAL (main_vbox);     /* Not wrapped with an API, can use BTK_DIALOG->vbox instead, -mr */
  BtkWidget *GSEAL (action_area);   /* Not wrapped with an API, can use BTK_DIALOG->action_area instead, -mr */

  /*< public >*/
  BtkWidget *GSEAL (ok_button);
  BtkWidget *GSEAL (apply_button);
  BtkWidget *GSEAL (cancel_button);

  /*< private >*/

  /* If the user changes the width of the dialog, we turn auto-shrink off.
   * (Unused now, autoshrink doesn't mean anything anymore -Yosh)
   */
  gint GSEAL (dialog_width);
  gboolean GSEAL (auto_resize);
};

struct _BtkFontSelectionDialogClass
{
  BtkDialogClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};



/*****************************************************************************
 * BtkFontSelection functions.
 *   see the comments in the BtkFontSelectionDialog functions.
 *****************************************************************************/

GType	     btk_font_selection_get_type	  (void) B_GNUC_CONST;
BtkWidget *  btk_font_selection_new               (void);
BtkWidget *  btk_font_selection_get_family_list   (BtkFontSelection *fontsel);
BtkWidget *  btk_font_selection_get_face_list     (BtkFontSelection *fontsel);
BtkWidget *  btk_font_selection_get_size_entry    (BtkFontSelection *fontsel);
BtkWidget *  btk_font_selection_get_size_list     (BtkFontSelection *fontsel);
BtkWidget *  btk_font_selection_get_preview_entry (BtkFontSelection *fontsel);
BangoFontFamily *
             btk_font_selection_get_family        (BtkFontSelection *fontsel);
BangoFontFace *
             btk_font_selection_get_face          (BtkFontSelection *fontsel);
gint         btk_font_selection_get_size          (BtkFontSelection *fontsel);
gchar*       btk_font_selection_get_font_name     (BtkFontSelection *fontsel);

#ifndef BTK_DISABLE_DEPRECATED
BdkFont*     btk_font_selection_get_font          (BtkFontSelection *fontsel);
#endif /* BTK_DISABLE_DEPRECATED */

gboolean     btk_font_selection_set_font_name     (BtkFontSelection *fontsel,
                                                   const gchar      *fontname);
const gchar* btk_font_selection_get_preview_text  (BtkFontSelection *fontsel);
void         btk_font_selection_set_preview_text  (BtkFontSelection *fontsel,
                                                   const gchar      *text);

/*****************************************************************************
 * BtkFontSelectionDialog functions.
 *   most of these functions simply call the corresponding function in the
 *   BtkFontSelection.
 *****************************************************************************/

GType	   btk_font_selection_dialog_get_type	       (void) B_GNUC_CONST;
BtkWidget *btk_font_selection_dialog_new	       (const gchar            *title);

BtkWidget *btk_font_selection_dialog_get_ok_button     (BtkFontSelectionDialog *fsd);
#ifndef BTK_DISABLE_DEPRECATED
BtkWidget *btk_font_selection_dialog_get_apply_button  (BtkFontSelectionDialog *fsd);
#endif
BtkWidget *btk_font_selection_dialog_get_cancel_button (BtkFontSelectionDialog *fsd);
BtkWidget *btk_font_selection_dialog_get_font_selection (BtkFontSelectionDialog *fsd);

/* This returns the X Logical Font Description fontname, or NULL if no font
   is selected. Note that there is a slight possibility that the font might not
   have been loaded OK. You should call btk_font_selection_dialog_get_font()
   to see if it has been loaded OK.
   You should g_free() the returned font name after you're done with it. */
gchar*	   btk_font_selection_dialog_get_font_name     (BtkFontSelectionDialog *fsd);

#ifndef BTK_DISABLE_DEPRECATED
/* This will return the current BdkFont, or NULL if none is selected or there
   was a problem loading it. Remember to use bdk_font_ref/unref() if you want
   to use the font (in a style, for example). */
BdkFont*   btk_font_selection_dialog_get_font	       (BtkFontSelectionDialog *fsd);
#endif /* BTK_DISABLE_DEPRECATED */

/* This sets the currently displayed font. It should be a valid X Logical
   Font Description font name (anything else will be ignored), e.g.
   "-adobe-courier-bold-o-normal--25-*-*-*-*-*-*-*" 
   It returns TRUE on success. */
gboolean   btk_font_selection_dialog_set_font_name     (BtkFontSelectionDialog *fsd,
                                                        const gchar	       *fontname);

/* This returns the text in the preview entry. You should copy the returned
   text if you need it. */
const gchar*
          btk_font_selection_dialog_get_preview_text   (BtkFontSelectionDialog *fsd);

/* This sets the text in the preview entry. It will be copied by the entry,
   so there's no need to g_strdup() it first. */
void	  btk_font_selection_dialog_set_preview_text   (BtkFontSelectionDialog *fsd,
                                                        const gchar	       *text);


B_END_DECLS


#endif /* __BTK_FONTSEL_H__ */
