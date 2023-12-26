/* BTK - The GIMP Toolkit

   Copyright (C) 2001 CodeFactory AB
   Copyright (C) 2001 Anders Carlsson <andersca@codefactory.se>
   Copyright (C) 2003, 2004 Matthias Clasen <mclasen@redhat.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Anders Carlsson <andersca@codefactory.se>
*/

#ifndef __BTK_ABOUT_DIALOG_H__
#define __BTK_ABOUT_DIALOG_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkdialog.h>

G_BEGIN_DECLS

#define BTK_TYPE_ABOUT_DIALOG            (btk_about_dialog_get_type ())
#define BTK_ABOUT_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_ABOUT_DIALOG, BtkAboutDialog))
#define BTK_ABOUT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ABOUT_DIALOG, BtkAboutDialogClass))
#define BTK_IS_ABOUT_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_ABOUT_DIALOG))
#define BTK_IS_ABOUT_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ABOUT_DIALOG))
#define BTK_ABOUT_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ABOUT_DIALOG, BtkAboutDialogClass))

typedef struct _BtkAboutDialog        BtkAboutDialog;
typedef struct _BtkAboutDialogClass   BtkAboutDialogClass;

/**
 * BtkAboutDialog:
 *
 * The <structname>BtkAboutDialog</structname> struct contains
 * only private fields and should not be directly accessed.
 */
struct _BtkAboutDialog 
{
  BtkDialog parent_instance;

  /*< private >*/
  gpointer GSEAL (private_data);
};

struct _BtkAboutDialogClass 
{
  BtkDialogClass parent_class;

  gboolean (*activate_link) (BtkAboutDialog *dialog,
                             const gchar    *uri);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};

GType                  btk_about_dialog_get_type               (void) G_GNUC_CONST;
BtkWidget             *btk_about_dialog_new                    (void);
void                   btk_show_about_dialog                   (BtkWindow       *parent,
								const gchar     *first_property_name,
								...) G_GNUC_NULL_TERMINATED;

#ifndef BTK_DISABLE_DEPRECATED
const gchar *          btk_about_dialog_get_name               (BtkAboutDialog  *about);
void                   btk_about_dialog_set_name               (BtkAboutDialog  *about,
								const gchar     *name);
#endif /* BTK_DISABLE_DEPRECATED */
const gchar *          btk_about_dialog_get_program_name       (BtkAboutDialog  *about);
void                   btk_about_dialog_set_program_name       (BtkAboutDialog  *about,
								const gchar     *name);
const gchar *          btk_about_dialog_get_version            (BtkAboutDialog  *about);
void                   btk_about_dialog_set_version            (BtkAboutDialog  *about,
								const gchar     *version);
const gchar *          btk_about_dialog_get_copyright          (BtkAboutDialog  *about);
void                   btk_about_dialog_set_copyright          (BtkAboutDialog  *about,
								const gchar     *copyright);
const gchar *          btk_about_dialog_get_comments           (BtkAboutDialog  *about);
void                   btk_about_dialog_set_comments           (BtkAboutDialog  *about,
								const gchar     *comments);
const gchar *          btk_about_dialog_get_license            (BtkAboutDialog  *about);
void                   btk_about_dialog_set_license            (BtkAboutDialog  *about,
								const gchar     *license);

gboolean               btk_about_dialog_get_wrap_license       (BtkAboutDialog  *about);
void                   btk_about_dialog_set_wrap_license       (BtkAboutDialog  *about,
                                                                gboolean         wrap_license);

const gchar *          btk_about_dialog_get_website            (BtkAboutDialog  *about);
void                   btk_about_dialog_set_website            (BtkAboutDialog  *about,
								const gchar     *website);
const gchar *          btk_about_dialog_get_website_label      (BtkAboutDialog  *about);
void                   btk_about_dialog_set_website_label      (BtkAboutDialog  *about,
								const gchar     *website_label);
const gchar* const *   btk_about_dialog_get_authors            (BtkAboutDialog  *about);
void                   btk_about_dialog_set_authors            (BtkAboutDialog  *about,
								const gchar    **authors);
const gchar* const *   btk_about_dialog_get_documenters        (BtkAboutDialog  *about);
void                   btk_about_dialog_set_documenters        (BtkAboutDialog  *about,
								const gchar    **documenters);
const gchar* const *   btk_about_dialog_get_artists            (BtkAboutDialog  *about);
void                   btk_about_dialog_set_artists            (BtkAboutDialog  *about,
								const gchar    **artists);
const gchar *          btk_about_dialog_get_translator_credits (BtkAboutDialog  *about);
void                   btk_about_dialog_set_translator_credits (BtkAboutDialog  *about,
								const gchar     *translator_credits);
BdkPixbuf             *btk_about_dialog_get_logo               (BtkAboutDialog  *about);
void                   btk_about_dialog_set_logo               (BtkAboutDialog  *about,
								BdkPixbuf       *logo);
const gchar *          btk_about_dialog_get_logo_icon_name     (BtkAboutDialog  *about);
void                   btk_about_dialog_set_logo_icon_name     (BtkAboutDialog  *about,
								const gchar     *icon_name);

/**
 * BtkAboutDialogActivateLinkFunc:
 * @about: the #BtkAboutDialog in which the link was activated
 * @link_: the URL or email address to which the activated link points
 * @data: user data that was passed when the function was registered
 *  with btk_about_dialog_set_email_hook() or
 *  btk_about_dialog_set_url_hook()
 *
 * The type of a function which is called when a URL or email
 * link is activated.
 */
typedef void (* BtkAboutDialogActivateLinkFunc) (BtkAboutDialog *about,
						 const gchar    *link_,
						 gpointer        data);

#ifndef BTK_DISABLE_DEPRECATED
BtkAboutDialogActivateLinkFunc btk_about_dialog_set_email_hook (BtkAboutDialogActivateLinkFunc func,
								gpointer                       data,
								GDestroyNotify                 destroy);
BtkAboutDialogActivateLinkFunc btk_about_dialog_set_url_hook   (BtkAboutDialogActivateLinkFunc func,
								gpointer                       data,
								GDestroyNotify                 destroy);
#endif

G_END_DECLS

#endif /* __BTK_ABOUT_DIALOG_H__ */


