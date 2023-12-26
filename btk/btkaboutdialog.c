/* BTK - The GIMP Toolkit
 * Copyright (C) 2001 CodeFactory AB
 * Copyright (C) 2001, 2002 Anders Carlsson
 * Copyright (C) 2003, 2004 Matthias Clasen <mclasen@redhat.com>
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
 */

/*
 * Author: Anders Carlsson <andersca@bunny.org>
 *
 * Modified by the BTK+ Team and others 1997-2004.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#include "config.h"

#include <string.h>

#include <bdk/bdkkeysyms.h>

#include "btkaboutdialog.h"
#include "btkbutton.h"
#include "btkbbox.h"
#include "btkdialog.h"
#include "btkhbox.h"
#include "btkimage.h"
#include "btklabel.h"
#include "btklinkbutton.h"
#include "btkmarshalers.h"
#include "btknotebook.h"
#include "btkscrolledwindow.h"
#include "btkstock.h"
#include "btktextview.h"
#include "btkvbox.h"
#include "btkiconfactory.h"
#include "btkshow.h"
#include "btkmain.h"
#include "btkmessagedialog.h"
#include "btkprivate.h"
#include "btkintl.h"

#include "btkalias.h"


/**
 * SECTION:btkaboutdialog
 * @Short_description: Display information about an application
 * @Title: BtkAboutDialog
 * @See_also:#BTK_STOCK_ABOUT
 *
 * The #BtkAboutDialog offers a simple way to display information about
 * a program like its logo, name, copyright, website and license. It is
 * also possible to give credits to the authors, documenters, translators
 * and artists who have worked on the program. An about dialog is typically
 * opened when the user selects the <literal>About</literal> option from
 * the <literal>Help</literal> menu. All parts of the dialog are optional.
 *
 * About dialog often contain links and email addresses. #BtkAboutDialog
 * supports this by offering global hooks, which are called when the user
 * clicks on a link or email address, see btk_about_dialog_set_email_hook()
 * and btk_about_dialog_set_url_hook(). Email addresses in the
 * authors, documenters and artists properties are recognized by looking for
 * <literal>&lt;user@<!-- -->host&gt;</literal>, URLs are
 * recognized by looking for <literal>http://url</literal>, with
 * <literal>url</literal> extending to the next space, tab or line break.
 *
 * <para id="btk-about-dialog-hook-setup">
 * Since 2.18 #BtkAboutDialog provides default website and email hooks that
 * use btk_show_uri().
 * </para>
 *
 * If you want provide your own hooks overriding the default ones, it is
 * important to do so before setting the website and email URL properties,
 * like this:
 * <informalexample><programlisting>
 * btk_about_dialog_set_url_hook (BTK_ABOUT_DIALOG (dialog), launch_url, NULL, NULL);
 * btk_about_dialog_set_website (BTK_ABOUT_DIALOG (dialog), app_url);
 * </programlisting></informalexample>
 * To disable the default hooks, you can pass %NULL as the hook func. Then,
 * the #BtkAboutDialog widget will not display the website or the
 * email addresses as clickable.
 *
 * To make constructing a #BtkAboutDialog as convenient as possible, you can
 * use the function btk_show_about_dialog() which constructs and shows a dialog
 * and keeps it around so that it can be shown again.
 *
 * Note that BTK+ sets a default title of <literal>_("About &percnt;s")</literal>
 * on the dialog window (where &percnt;s is replaced by the name of the
 * application, but in order to ensure proper translation of the title,
 * applications should set the title property explicitly when constructing
 * a #BtkAboutDialog, as shown in the following example:
 * <informalexample><programlisting>
 * btk_show_about_dialog (NULL,
 *                        "program-name", "ExampleCode",
 *                        "logo", example_logo,
 *                        "title" _("About ExampleCode"),
 *                        NULL);
 * </programlisting></informalexample>
 * Note that prior to BTK+ 2.12, the #BtkAboutDialog:program-name property
 * was called "name". This was changed to avoid the conflict with the
 * #BtkWidget:name property.
 */


static BdkColor default_link_color = { 0, 0, 0, 0xeeee };
static BdkColor default_visited_link_color = { 0, 0x5555, 0x1a1a, 0x8b8b };

typedef struct _BtkAboutDialogPrivate BtkAboutDialogPrivate;
struct _BtkAboutDialogPrivate
{
  bchar *name;
  bchar *version;
  bchar *copyright;
  bchar *comments;
  bchar *website_url;
  bchar *website_text;
  bchar *translator_credits;
  bchar *license;

  bchar **authors;
  bchar **documenters;
  bchar **artists;

  BtkWidget *logo_image;
  BtkWidget *name_label;
  BtkWidget *comments_label;
  BtkWidget *copyright_label;
  BtkWidget *website_label;

  BtkWidget *credits_button;
  BtkWidget *credits_dialog;
  BtkWidget *license_button;
  BtkWidget *license_dialog;

  BdkCursor *hand_cursor;
  BdkCursor *regular_cursor;

  GSList *visited_links;

  buint hovering_over_link : 1;
  buint wrap_license : 1;
};

#define BTK_ABOUT_DIALOG_GET_PRIVATE(obj) (B_TYPE_INSTANCE_GET_PRIVATE ((obj), BTK_TYPE_ABOUT_DIALOG, BtkAboutDialogPrivate))


enum
{
  PROP_0,
  PROP_NAME,
  PROP_VERSION,
  PROP_COPYRIGHT,
  PROP_COMMENTS,
  PROP_WEBSITE,
  PROP_WEBSITE_LABEL,
  PROP_LICENSE,
  PROP_AUTHORS,
  PROP_DOCUMENTERS,
  PROP_TRANSLATOR_CREDITS,
  PROP_ARTISTS,
  PROP_LOGO,
  PROP_LOGO_ICON_NAME,
  PROP_WRAP_LICENSE
};

static void                 btk_about_dialog_finalize       (BObject            *object);
static void                 btk_about_dialog_get_property   (BObject            *object,
                                                             buint               prop_id,
                                                             BValue             *value,
                                                             BParamSpec         *pspec);
static void                 btk_about_dialog_set_property   (BObject            *object,
                                                             buint               prop_id,
                                                             const BValue       *value,
                                                             BParamSpec         *pspec);
static void                 btk_about_dialog_show           (BtkWidget          *widge);
static void                 update_name_version             (BtkAboutDialog     *about);
static BtkIconSet *         icon_set_new_from_pixbufs       (GList              *pixbufs);
static void                 follow_if_link                  (BtkAboutDialog     *about,
                                                             BtkTextView        *text_view,
                                                             BtkTextIter        *iter);
static void                 set_cursor_if_appropriate       (BtkAboutDialog     *about,
                                                             BtkTextView        *text_view,
                                                             bint                x,
                                                             bint                y);
static void                 display_credits_dialog          (BtkWidget          *button,
                                                             bpointer            data);
static void                 display_license_dialog          (BtkWidget          *button,
                                                             bpointer            data);
static void                 close_cb                        (BtkAboutDialog     *about);
static bboolean             btk_about_dialog_activate_link  (BtkAboutDialog     *about,
                                                             const bchar        *uri);

static void                 default_url_hook                (BtkAboutDialog     *about,
                                                             const bchar        *uri,
                                                             bpointer            user_data);
static void                 default_email_hook              (BtkAboutDialog     *about,
                                                             const bchar        *email_address,
                                                             bpointer            user_data);

static bboolean activate_email_hook_set = FALSE;
static BtkAboutDialogActivateLinkFunc activate_email_hook = NULL;
static bpointer activate_email_hook_data = NULL;
static GDestroyNotify activate_email_hook_destroy = NULL;

static bboolean activate_url_hook_set = FALSE;
static BtkAboutDialogActivateLinkFunc activate_url_hook = NULL;
static bpointer activate_url_hook_data = NULL;
static GDestroyNotify activate_url_hook_destroy = NULL;

static void
default_url_hook (BtkAboutDialog *about,
                  const bchar    *uri,
                  bpointer        user_data)
{
  BdkScreen *screen;
  GError *error = NULL;

  screen = btk_widget_get_screen (BTK_WIDGET (about));

  if (!btk_show_uri (screen, uri, btk_get_current_event_time (), &error))
    {
      BtkWidget *dialog;

      dialog = btk_message_dialog_new (BTK_WINDOW (about),
                                       BTK_DIALOG_DESTROY_WITH_PARENT |
                                       BTK_DIALOG_MODAL,
                                       BTK_MESSAGE_ERROR,
                                       BTK_BUTTONS_CLOSE,
                                       "%s", _("Could not show link"));
      btk_message_dialog_format_secondary_text (BTK_MESSAGE_DIALOG (dialog),
                                                "%s", error->message);
      g_error_free (error);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (btk_widget_destroy), NULL);

      btk_window_present (BTK_WINDOW (dialog));
    }
}

static void
default_email_hook (BtkAboutDialog *about,
                    const bchar    *email_address,
                    bpointer        user_data)
{
  char *escaped, *uri;

  escaped = g_uri_escape_string (email_address, NULL, FALSE);
  uri = g_strdup_printf ("mailto:%s", escaped);
  g_free (escaped);

  default_url_hook (about, uri, user_data);
  g_free (uri);
}

enum {
  ACTIVATE_LINK,
  LAST_SIGNAL
};

static buint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BtkAboutDialog, btk_about_dialog, BTK_TYPE_DIALOG)

static void
btk_about_dialog_class_init (BtkAboutDialogClass *klass)
{
  BObjectClass *object_class;
  BtkWidgetClass *widget_class;

  object_class = (BObjectClass *)klass;
  widget_class = (BtkWidgetClass *)klass;

  object_class->set_property = btk_about_dialog_set_property;
  object_class->get_property = btk_about_dialog_get_property;

  object_class->finalize = btk_about_dialog_finalize;

  widget_class->show = btk_about_dialog_show;

  klass->activate_link = btk_about_dialog_activate_link;

  /**
   * BtkAboutDialog::activate-link:
   * @label: The object on which the signal was emitted
   * @uri: the URI that is activated
   *
   * The signal which gets emitted to activate a URI.
   * Applications may connect to it to override the default behaviour,
   * which is to call btk_show_uri().
   *
   * Returns: %TRUE if the link has been activated
   *
   * Since: 2.24
   */
  signals[ACTIVATE_LINK] =
    g_signal_new ("activate-link",
                  B_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BtkAboutDialogClass, activate_link),
                  _btk_boolean_handled_accumulator, NULL,
                  _btk_marshal_BOOLEAN__STRING,
                  B_TYPE_BOOLEAN, 1, B_TYPE_STRING);

  /**
   * BtkAboutDialog:program-name:
   *
   * The name of the program.
   * If this is not set, it defaults to g_get_application_name().
   *
   * Since: 2.12
   */
  g_object_class_install_property (object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("program-name",
                                                        P_("Program name"),
                                                        P_("The name of the program. If this is not set, it defaults to g_get_application_name()"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:version:
   *
   * The version of the program.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_VERSION,
                                   g_param_spec_string ("version",
                                                        P_("Program version"),
                                                        P_("The version of the program"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:copyright:
   *
   * Copyright information for the program.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_COPYRIGHT,
                                   g_param_spec_string ("copyright",
                                                        P_("Copyright string"),
                                                        P_("Copyright information for the program"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
        

  /**
   * BtkAboutDialog:comments:
   *
   * Comments about the program. This string is displayed in a label
   * in the main dialog, thus it should be a short explanation of
   * the main purpose of the program, not a detailed list of features.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_COMMENTS,
                                   g_param_spec_string ("comments",
                                                        P_("Comments string"),
                                                        P_("Comments about the program"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:license:
   *
   * The license of the program. This string is displayed in a
   * text view in a secondary dialog, therefore it is fine to use
   * a long multi-paragraph text. Note that the text is only wrapped
   * in the text view if the "wrap-license" property is set to %TRUE;
   * otherwise the text itself must contain the intended linebreaks.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_LICENSE,
                                   g_param_spec_string ("license",
                                                        _("License"),
                                                        _("The license of the program"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:website:
   *
   * The URL for the link to the website of the program.
   * This should be a string starting with "http://.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_WEBSITE,
                                   g_param_spec_string ("website",
                                                        P_("Website URL"),
                                                        P_("The URL for the link to the website of the program"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:website-label:
   *
   * The label for the link to the website of the program. If this is not set,
   * it defaults to the URL specified in the #BtkAboutDialog:website property.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_WEBSITE_LABEL,
                                   g_param_spec_string ("website-label",
                                                        P_("Website label"),
                                                        P_("The label for the link to the website of the program. If this is not set, it defaults to the URL"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:authors:
   *
   * The authors of the program, as a %NULL-terminated array of strings.
   * Each string may contain email addresses and URLs, which will be displayed
   * as links, see the introduction for more details.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_AUTHORS,
                                   g_param_spec_boxed ("authors",
                                                       P_("Authors"),
                                                       P_("List of authors of the program"),
                                                       B_TYPE_STRV,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:documenters:
   *
   * The people documenting the program, as a %NULL-terminated array of strings.
   * Each string may contain email addresses and URLs, which will be displayed
   * as links, see the introduction for more details.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_DOCUMENTERS,
                                   g_param_spec_boxed ("documenters",
                                                       P_("Documenters"),
                                                       P_("List of people documenting the program"),
                                                       B_TYPE_STRV,
                                                       BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:artists:
   *
   * The people who contributed artwork to the program, as a %NULL-terminated
   * array of strings. Each string may contain email addresses and URLs, which
   * will be displayed as links, see the introduction for more details.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_ARTISTS,
                                   g_param_spec_boxed ("artists",
                                                       P_("Artists"),
                                                       P_("List of people who have contributed artwork to the program"),
                                                       B_TYPE_STRV,
                                                       BTK_PARAM_READWRITE));


  /**
   * BtkAboutDialog:translator-credits:
   *
   * Credits to the translators. This string should be marked as translatable.
   * The string may contain email addresses and URLs, which will be displayed
   * as links, see the introduction for more details.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_TRANSLATOR_CREDITS,
                                   g_param_spec_string ("translator-credits",
                                                        P_("Translator credits"),
                                                        P_("Credits to the translators. This string should be marked as translatable"),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
        
  /**
   * BtkAboutDialog:logo:
   *
   * A logo for the about box. If this is not set, it defaults to
   * btk_window_get_default_icon_list().
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_LOGO,
                                   g_param_spec_object ("logo",
                                                        P_("Logo"),
                                                        P_("A logo for the about box. If this is not set, it defaults to btk_window_get_default_icon_list()"),
                                                        BDK_TYPE_PIXBUF,
                                                        BTK_PARAM_READWRITE));

  /**
   * BtkAboutDialog:logo-icon-name:
   *
   * A named icon to use as the logo for the about box. This property
   * overrides the #BtkAboutDialog:logo property.
   *
   * Since: 2.6
   */
  g_object_class_install_property (object_class,
                                   PROP_LOGO_ICON_NAME,
                                   g_param_spec_string ("logo-icon-name",
                                                        P_("Logo Icon Name"),
                                                        P_("A named icon to use as the logo for the about box."),
                                                        NULL,
                                                        BTK_PARAM_READWRITE));
  /**
   * BtkAboutDialog:wrap-license:
   *
   * Whether to wrap the text in the license dialog.
   *
   * Since: 2.8
   */
  g_object_class_install_property (object_class,
                                   PROP_WRAP_LICENSE,
                                   g_param_spec_boolean ("wrap-license",
                                                         P_("Wrap license"),
                                                         P_("Whether to wrap the license text."),
                                                         FALSE,
                                                         BTK_PARAM_READWRITE));


  g_type_class_add_private (object_class, sizeof (BtkAboutDialogPrivate));
}

static bboolean
emit_activate_link (BtkAboutDialog *about,
                    const bchar    *uri)
{
  bboolean handled = FALSE;

  g_signal_emit (about, signals[ACTIVATE_LINK], 0, uri, &handled);

  return TRUE;
}

static void
btk_about_dialog_init (BtkAboutDialog *about)
{
  BtkDialog *dialog = BTK_DIALOG (about);
  BtkAboutDialogPrivate *priv;
  BtkWidget *vbox, *hbox, *button, *close_button, *image;

  /* Data */
  priv = BTK_ABOUT_DIALOG_GET_PRIVATE (about);
  about->private_data = priv;

  priv->name = NULL;
  priv->version = NULL;
  priv->copyright = NULL;
  priv->comments = NULL;
  priv->website_url = NULL;
  priv->website_text = NULL;
  priv->translator_credits = NULL;
  priv->license = NULL;
  priv->authors = NULL;
  priv->documenters = NULL;
  priv->artists = NULL;

  priv->hand_cursor = bdk_cursor_new (BDK_HAND2);
  priv->regular_cursor = bdk_cursor_new (BDK_XTERM);
  priv->hovering_over_link = FALSE;
  priv->wrap_license = FALSE;

  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);

  /* Widgets */
  btk_widget_push_composite_child ();

  vbox = btk_vbox_new (FALSE, 8);
  btk_container_set_border_width (BTK_CONTAINER (vbox), 5);
  btk_box_pack_start (BTK_BOX (dialog->vbox), vbox, TRUE, TRUE, 0);

  priv->logo_image = btk_image_new ();
  btk_box_pack_start (BTK_BOX (vbox), priv->logo_image, FALSE, FALSE, 0);

  priv->name_label = btk_label_new (NULL);
  btk_label_set_selectable (BTK_LABEL (priv->name_label), TRUE);
  btk_label_set_justify (BTK_LABEL (priv->name_label), BTK_JUSTIFY_CENTER);
  btk_box_pack_start (BTK_BOX (vbox), priv->name_label, FALSE, FALSE, 0);

  priv->comments_label = btk_label_new (NULL);
  btk_label_set_selectable (BTK_LABEL (priv->comments_label), TRUE);
  btk_label_set_justify (BTK_LABEL (priv->comments_label), BTK_JUSTIFY_CENTER);
  btk_label_set_line_wrap (BTK_LABEL (priv->comments_label), TRUE);
  btk_box_pack_start (BTK_BOX (vbox), priv->comments_label, FALSE, FALSE, 0);

  priv->copyright_label = btk_label_new (NULL);
  btk_label_set_selectable (BTK_LABEL (priv->copyright_label), TRUE);
  btk_label_set_justify (BTK_LABEL (priv->copyright_label), BTK_JUSTIFY_CENTER);
  btk_box_pack_start (BTK_BOX (vbox), priv->copyright_label, FALSE, FALSE, 0);

  hbox = btk_hbox_new (TRUE, 0);
  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  priv->website_label = button = btk_label_new ("");
  btk_widget_set_no_show_all (button, TRUE);
  btk_label_set_selectable (BTK_LABEL (button), TRUE);
  btk_box_pack_start (BTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect_swapped (button, "activate-link",
                            G_CALLBACK (emit_activate_link), about);

  btk_widget_show (vbox);
  btk_widget_show (priv->logo_image);
  btk_widget_show (priv->name_label);
  btk_widget_show (hbox);

  /* Add the close button */
  close_button = btk_dialog_add_button (BTK_DIALOG (about), BTK_STOCK_CLOSE,
                                        BTK_RESPONSE_CANCEL);
  btk_dialog_set_default_response (BTK_DIALOG (about), BTK_RESPONSE_CANCEL);

  /* Add the credits button */
  button = btk_button_new_with_mnemonic (_("C_redits"));
  btk_widget_set_can_default (button, TRUE);
  image = btk_image_new_from_stock (BTK_STOCK_ABOUT, BTK_ICON_SIZE_BUTTON);
  btk_button_set_image (BTK_BUTTON (button), image);
  btk_widget_set_no_show_all (button, TRUE);
  btk_box_pack_end (BTK_BOX (BTK_DIALOG (about)->action_area),
                    button, FALSE, TRUE, 0);
  btk_button_box_set_child_secondary (BTK_BUTTON_BOX (BTK_DIALOG (about)->action_area), button, TRUE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (display_credits_dialog), about);
  priv->credits_button = button;
  priv->credits_dialog = NULL;

  /* Add the license button */
  button = btk_button_new_from_stock (_("_License"));
  btk_widget_set_can_default (button, TRUE);
  btk_widget_set_no_show_all (button, TRUE);
  btk_box_pack_end (BTK_BOX (BTK_DIALOG (about)->action_area),
                    button, FALSE, TRUE, 0);
  btk_button_box_set_child_secondary (BTK_BUTTON_BOX (BTK_DIALOG (about)->action_area), button, TRUE);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (display_license_dialog), about);
  priv->license_button = button;
  priv->license_dialog = NULL;

  btk_window_set_resizable (BTK_WINDOW (about), FALSE);

  btk_widget_pop_composite_child ();

  btk_widget_grab_default (close_button);
  btk_widget_grab_focus (close_button);

  /* force defaults */
  btk_about_dialog_set_program_name (about, NULL);
  btk_about_dialog_set_logo (about, NULL);
}

static void
btk_about_dialog_finalize (BObject *object)
{
  BtkAboutDialog *about = BTK_ABOUT_DIALOG (object);
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;

  g_free (priv->name);
  g_free (priv->version);
  g_free (priv->copyright);
  g_free (priv->comments);
  g_free (priv->license);
  g_free (priv->website_url);
  g_free (priv->website_text);
  g_free (priv->translator_credits);

  g_strfreev (priv->authors);
  g_strfreev (priv->documenters);
  g_strfreev (priv->artists);

  b_slist_foreach (priv->visited_links, (GFunc)g_free, NULL);
  b_slist_free (priv->visited_links);

  bdk_cursor_unref (priv->hand_cursor);
  bdk_cursor_unref (priv->regular_cursor);

  B_OBJECT_CLASS (btk_about_dialog_parent_class)->finalize (object);
}

static void
btk_about_dialog_set_property (BObject      *object,
                               buint         prop_id,
                               const BValue *value,
                               BParamSpec   *pspec)
{
  BtkAboutDialog *about = BTK_ABOUT_DIALOG (object);
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;

  switch (prop_id)
    {
    case PROP_NAME:
      btk_about_dialog_set_program_name (about, b_value_get_string (value));
      break;
    case PROP_VERSION:
      btk_about_dialog_set_version (about, b_value_get_string (value));
      break;
    case PROP_COMMENTS:
      btk_about_dialog_set_comments (about, b_value_get_string (value));
      break;
    case PROP_WEBSITE:
      btk_about_dialog_set_website (about, b_value_get_string (value));
      break;
    case PROP_WEBSITE_LABEL:
      btk_about_dialog_set_website_label (about, b_value_get_string (value));
      break;
    case PROP_LICENSE:
      btk_about_dialog_set_license (about, b_value_get_string (value));
      break;
    case PROP_COPYRIGHT:
      btk_about_dialog_set_copyright (about, b_value_get_string (value));
      break;
    case PROP_LOGO:
      btk_about_dialog_set_logo (about, b_value_get_object (value));
      break;
    case PROP_AUTHORS:
      btk_about_dialog_set_authors (about, (const bchar**)b_value_get_boxed (value));
      break;
    case PROP_DOCUMENTERS:
      btk_about_dialog_set_documenters (about, (const bchar**)b_value_get_boxed (value));
      break;
    case PROP_ARTISTS:
      btk_about_dialog_set_artists (about, (const bchar**)b_value_get_boxed (value));
      break;
    case PROP_TRANSLATOR_CREDITS:
      btk_about_dialog_set_translator_credits (about, b_value_get_string (value));
      break;
    case PROP_LOGO_ICON_NAME:
      btk_about_dialog_set_logo_icon_name (about, b_value_get_string (value));
      break;
    case PROP_WRAP_LICENSE:
      priv->wrap_license = b_value_get_boolean (value);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
btk_about_dialog_get_property (BObject    *object,
                               buint       prop_id,
                               BValue     *value,
                               BParamSpec *pspec)
{
  BtkAboutDialog *about = BTK_ABOUT_DIALOG (object);
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;

  switch (prop_id)
    {
    case PROP_NAME:
      b_value_set_string (value, priv->name);
      break;
    case PROP_VERSION:
      b_value_set_string (value, priv->version);
      break;
    case PROP_COPYRIGHT:
      b_value_set_string (value, priv->copyright);
      break;
    case PROP_COMMENTS:
      b_value_set_string (value, priv->comments);
      break;
    case PROP_WEBSITE:
      b_value_set_string (value, priv->website_url);
      break;
    case PROP_WEBSITE_LABEL:
      b_value_set_string (value, priv->website_text);
      break;
    case PROP_LICENSE:
      b_value_set_string (value, priv->license);
      break;
    case PROP_TRANSLATOR_CREDITS:
      b_value_set_string (value, priv->translator_credits);
      break;
    case PROP_AUTHORS:
      b_value_set_boxed (value, priv->authors);
      break;
    case PROP_DOCUMENTERS:
      b_value_set_boxed (value, priv->documenters);
      break;
    case PROP_ARTISTS:
      b_value_set_boxed (value, priv->artists);
      break;
    case PROP_LOGO:
      if (btk_image_get_storage_type (BTK_IMAGE (priv->logo_image)) == BTK_IMAGE_PIXBUF)
        b_value_set_object (value, btk_image_get_pixbuf (BTK_IMAGE (priv->logo_image)));
      else
        b_value_set_object (value, NULL);
      break;
    case PROP_LOGO_ICON_NAME:
      if (btk_image_get_storage_type (BTK_IMAGE (priv->logo_image)) == BTK_IMAGE_ICON_NAME)
        {
          const bchar *icon_name;

          btk_image_get_icon_name (BTK_IMAGE (priv->logo_image), &icon_name, NULL);
          b_value_set_string (value, icon_name);
        }
      else
        b_value_set_string (value, NULL);
      break;
    case PROP_WRAP_LICENSE:
      b_value_set_boolean (value, priv->wrap_license);
      break;
    default:
      B_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static bboolean
btk_about_dialog_activate_link (BtkAboutDialog *about,
                                const bchar    *uri)
{
  if (g_str_has_prefix (uri, "mailto:"))
    {
      bchar *email;

      email = g_uri_unescape_string (uri + strlen ("mailto:"), NULL);

      if (activate_email_hook_set)
        activate_email_hook (about, email, activate_email_hook_data);
      else
        default_email_hook (about, email, NULL);

      g_free (email);
    }
  else
    {
      if (activate_url_hook_set)
        activate_url_hook (about, uri, activate_url_hook_data);
      else
        default_url_hook (about, uri, NULL);
    }

  return TRUE;
}

static void
update_website (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;

  btk_widget_show (priv->website_label);

  if (priv->website_url && (!activate_url_hook_set || activate_url_hook != NULL))
    {
      bchar *markup;

      if (priv->website_text)
        {
          bchar *escaped;

          escaped = g_markup_escape_text (priv->website_text, -1);
          markup = g_strdup_printf ("<a href=\"%s\">%s</a>",
                                    priv->website_url, escaped);
          g_free (escaped);
        }
      else
        {
          markup = g_strdup_printf ("<a href=\"%s\">%s</a>",
                                    priv->website_url, priv->website_url);
        }

      btk_label_set_markup (BTK_LABEL (priv->website_label), markup);
      g_free (markup);
    }
  else
    {
      if (priv->website_url)
        btk_label_set_text (BTK_LABEL (priv->website_label), priv->website_url);
      else if (priv->website_text)
        btk_label_set_text (BTK_LABEL (priv->website_label), priv->website_text);
      else
        btk_widget_hide (priv->website_label);
    }
}

static void
btk_about_dialog_show (BtkWidget *widget)
{
  update_website (BTK_ABOUT_DIALOG (widget));

  BTK_WIDGET_CLASS (btk_about_dialog_parent_class)->show (widget);
}

/**
 * btk_about_dialog_get_name:
 * @about: a #BtkAboutDialog
 *
 * Returns the program name displayed in the about dialog.
 *
 * Return value: The program name. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.6
 *
 * Deprecated: 2.12: Use btk_about_dialog_get_program_name() instead.
 */
const bchar *
btk_about_dialog_get_name (BtkAboutDialog *about)
{
  return btk_about_dialog_get_program_name (about);
}

/**
 * btk_about_dialog_get_program_name:
 * @about: a #BtkAboutDialog
 *
 * Returns the program name displayed in the about dialog.
 *
 * Return value: The program name. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.12
 */
const bchar *
btk_about_dialog_get_program_name (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->name;
}

static void
update_name_version (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;
  bchar *title_string, *name_string;

  priv = (BtkAboutDialogPrivate *)about->private_data;

  title_string = g_strdup_printf (_("About %s"), priv->name);
  btk_window_set_title (BTK_WINDOW (about), title_string);
  g_free (title_string);

  if (priv->version != NULL)
    name_string = g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s %s</span>",
                                             priv->name, priv->version);
  else
    name_string = g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>",
                                           priv->name);

  btk_label_set_markup (BTK_LABEL (priv->name_label), name_string);

  g_free (name_string);
}

/**
 * btk_about_dialog_set_name:
 * @about: a #BtkAboutDialog
 * @name: (allow-none): the program name
 *
 * Sets the name to display in the about dialog.
 * If this is not set, it defaults to g_get_application_name().
 *
 * Since: 2.6
 *
 * Deprecated: 2.12: Use btk_about_dialog_set_program_name() instead.
 */
void
btk_about_dialog_set_name (BtkAboutDialog *about,
                           const bchar    *name)
{
    btk_about_dialog_set_program_name (about, name);
}

/**
 * btk_about_dialog_set_program_name:
 * @about: a #BtkAboutDialog
 * @name: the program name
 *
 * Sets the name to display in the about dialog.
 * If this is not set, it defaults to g_get_application_name().
 *
 * Since: 2.12
 */
void
btk_about_dialog_set_program_name (BtkAboutDialog *about,
                                   const bchar    *name)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;
  tmp = priv->name;
  priv->name = g_strdup (name ? name : g_get_application_name ());
  g_free (tmp);

  update_name_version (about);

  g_object_notify (B_OBJECT (about), "program-name");
}


/**
 * btk_about_dialog_get_version:
 * @about: a #BtkAboutDialog
 *
 * Returns the version string.
 *
 * Return value: The version string. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_version (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->version;
}

/**
 * btk_about_dialog_set_version:
 * @about: a #BtkAboutDialog
 * @version: (allow-none): the version string
 *
 * Sets the version string to display in the about dialog.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_version (BtkAboutDialog *about,
                              const bchar    *version)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->version;
  priv->version = g_strdup (version);
  g_free (tmp);

  update_name_version (about);

  g_object_notify (B_OBJECT (about), "version");
}

/**
 * btk_about_dialog_get_copyright:
 * @about: a #BtkAboutDialog
 *
 * Returns the copyright string.
 *
 * Return value: The copyright string. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_copyright (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->copyright;
}

/**
 * btk_about_dialog_set_copyright:
 * @about: a #BtkAboutDialog
 * @copyright: (allow-none) the copyright string
 *
 * Sets the copyright string to display in the about dialog.
 * This should be a short string of one or two lines.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_copyright (BtkAboutDialog *about,
                                const bchar    *copyright)
{
  BtkAboutDialogPrivate *priv;
  bchar *copyright_string, *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->copyright;
  priv->copyright = g_strdup (copyright);
  g_free (tmp);

  if (priv->copyright != NULL)
    {
      copyright_string = g_markup_printf_escaped ("<span size=\"small\">%s</span>",
                                                  priv->copyright);
      btk_label_set_markup (BTK_LABEL (priv->copyright_label), copyright_string);
      g_free (copyright_string);

      btk_widget_show (priv->copyright_label);
    }
  else
    btk_widget_hide (priv->copyright_label);

  g_object_notify (B_OBJECT (about), "copyright");
}

/**
 * btk_about_dialog_get_comments:
 * @about: a #BtkAboutDialog
 *
 * Returns the comments string.
 *
 * Return value: The comments. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_comments (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->comments;
}

/**
 * btk_about_dialog_set_comments:
 * @about: a #BtkAboutDialog
 * @comments: (allow-none): a comments string
 *
 * Sets the comments string to display in the about dialog.
 * This should be a short string of one or two lines.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_comments (BtkAboutDialog *about,
                               const bchar    *comments)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->comments;
  if (comments)
    {
      priv->comments = g_strdup (comments);
      btk_label_set_text (BTK_LABEL (priv->comments_label), priv->comments);
      btk_widget_show (priv->comments_label);
    }
  else
    {
      priv->comments = NULL;
      btk_widget_hide (priv->comments_label);
    }
  g_free (tmp);

  g_object_notify (B_OBJECT (about), "comments");
}

/**
 * btk_about_dialog_get_license:
 * @about: a #BtkAboutDialog
 *
 * Returns the license information.
 *
 * Return value: The license information. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_license (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->license;
}

/**
 * btk_about_dialog_set_license:
 * @about: a #BtkAboutDialog
 * @license: (allow-none): the license information or %NULL
 *
 * Sets the license information to be displayed in the secondary
 * license dialog. If @license is %NULL, the license button is
 * hidden.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_license (BtkAboutDialog *about,
                              const bchar    *license)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->license;
  if (license)
    {
      priv->license = g_strdup (license);
      btk_widget_show (priv->license_button);
    }
  else
    {
      priv->license = NULL;
      btk_widget_hide (priv->license_button);
    }
  g_free (tmp);

  g_object_notify (B_OBJECT (about), "license");
}

/**
 * btk_about_dialog_get_wrap_license:
 * @about: a #BtkAboutDialog
 *
 * Returns whether the license text in @about is
 * automatically wrapped.
 *
 * Returns: %TRUE if the license text is wrapped
 *
 * Since: 2.8
 */
bboolean
btk_about_dialog_get_wrap_license (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), FALSE);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->wrap_license;
}

/**
 * btk_about_dialog_set_wrap_license:
 * @about: a #BtkAboutDialog
 * @wrap_license: whether to wrap the license
 *
 * Sets whether the license text in @about is
 * automatically wrapped.
 *
 * Since: 2.8
 */
void
btk_about_dialog_set_wrap_license (BtkAboutDialog *about,
                                   bboolean        wrap_license)
{
  BtkAboutDialogPrivate *priv;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  wrap_license = wrap_license != FALSE;

  if (priv->wrap_license != wrap_license)
    {
       priv->wrap_license = wrap_license;

       g_object_notify (B_OBJECT (about), "wrap-license");
    }
}

/**
 * btk_about_dialog_get_website:
 * @about: a #BtkAboutDialog
 *
 * Returns the website URL.
 *
 * Return value: The website URL. The string is owned by the about
 *  dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_website (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->website_url;
}

/**
 * btk_about_dialog_set_website:
 * @about: a #BtkAboutDialog
 * @website: (allow-none): a URL string starting with "http://"
 *
 * Sets the URL to use for the website link.
 *
 * Note that that the hook functions need to be set up
 * before calling this function.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_website (BtkAboutDialog *about,
                              const bchar    *website)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->website_url;
  priv->website_url = g_strdup (website);
  g_free (tmp);

  update_website (about);

  g_object_notify (B_OBJECT (about), "website");
}

/**
 * btk_about_dialog_get_website_label:
 * @about: a #BtkAboutDialog
 *
 * Returns the label used for the website link.
 *
 * Return value: The label used for the website link. The string is
 *     owned by the about dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_website_label (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->website_text;
}

/**
 * btk_about_dialog_set_website_label:
 * @about: a #BtkAboutDialog
 * @website_label: the label used for the website link
 *
 * Sets the label to be used for the website link.
 * It defaults to the website URL.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_website_label (BtkAboutDialog *about,
                                    const bchar    *website_label)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->website_text;
  priv->website_text = g_strdup (website_label);
  g_free (tmp);

  update_website (about);

  g_object_notify (B_OBJECT (about), "website-label");
}

/**
 * btk_about_dialog_get_authors:
 * @about: a #BtkAboutDialog
 *
 * Returns the string which are displayed in the authors tab
 * of the secondary credits dialog.
 *
 * Return value: (array zero-terminated=1) (transfer none): A
 *  %NULL-terminated string array containing the authors. The array is
 *  owned by the about dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar * const *
btk_about_dialog_get_authors (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return (const bchar * const *) priv->authors;
}

static void
update_credits_button_visibility (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv = about->private_data;
  bboolean show;

  show = priv->authors != NULL ||
         priv->documenters != NULL ||
         priv->artists != NULL ||
         (priv->translator_credits != NULL &&
          strcmp (priv->translator_credits, "translator_credits") &&
          strcmp (priv->translator_credits, "translator-credits"));
  if (show)
    btk_widget_show (priv->credits_button);
  else
    btk_widget_hide (priv->credits_button);
}

/**
 * btk_about_dialog_set_authors:
 * @about: a #BtkAboutDialog
 * @authors: a %NULL-terminated array of strings
 *
 * Sets the strings which are displayed in the authors tab
 * of the secondary credits dialog.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_authors (BtkAboutDialog  *about,
                              const bchar    **authors)
{
  BtkAboutDialogPrivate *priv;
  bchar **tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->authors;
  priv->authors = g_strdupv ((bchar **)authors);
  g_strfreev (tmp);

  update_credits_button_visibility (about);

  g_object_notify (B_OBJECT (about), "authors");
}

/**
 * btk_about_dialog_get_documenters:
 * @about: a #BtkAboutDialog
 *
 * Returns the string which are displayed in the documenters
 * tab of the secondary credits dialog.
 *
 * Return value: (array zero-terminated=1) (transfer none): A
 *  %NULL-terminated string array containing the documenters. The
 *  array is owned by the about dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar * const *
btk_about_dialog_get_documenters (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return (const bchar * const *)priv->documenters;
}

/**
 * btk_about_dialog_set_documenters:
 * @about: a #BtkAboutDialog
 * @documenters: a %NULL-terminated array of strings
 *
 * Sets the strings which are displayed in the documenters tab
 * of the secondary credits dialog.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_documenters (BtkAboutDialog *about,
                                  const bchar   **documenters)
{
  BtkAboutDialogPrivate *priv;
  bchar **tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->documenters;
  priv->documenters = g_strdupv ((bchar **)documenters);
  g_strfreev (tmp);

  update_credits_button_visibility (about);

  g_object_notify (B_OBJECT (about), "documenters");
}

/**
 * btk_about_dialog_get_artists:
 * @about: a #BtkAboutDialog
 *
 * Returns the string which are displayed in the artists tab
 * of the secondary credits dialog.
 *
 * Return value: (array zero-terminated=1) (transfer none): A
 *  %NULL-terminated string array containing the artists. The array is
 *  owned by the about dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar * const *
btk_about_dialog_get_artists (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return (const bchar * const *)priv->artists;
}

/**
 * btk_about_dialog_set_artists:
 * @about: a #BtkAboutDialog
 * @artists: a %NULL-terminated array of strings
 *
 * Sets the strings which are displayed in the artists tab
 * of the secondary credits dialog.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_artists (BtkAboutDialog *about,
                              const bchar   **artists)
{
  BtkAboutDialogPrivate *priv;
  bchar **tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->artists;
  priv->artists = g_strdupv ((bchar **)artists);
  g_strfreev (tmp);

  update_credits_button_visibility (about);

  g_object_notify (B_OBJECT (about), "artists");
}

/**
 * btk_about_dialog_get_translator_credits:
 * @about: a #BtkAboutDialog
 *
 * Returns the translator credits string which is displayed
 * in the translators tab of the secondary credits dialog.
 *
 * Return value: The translator credits string. The string is
 *   owned by the about dialog and must not be modified.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_translator_credits (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  return priv->translator_credits;
}

/**
 * btk_about_dialog_set_translator_credits:
 * @about: a #BtkAboutDialog
 * @translator_credits: (allow-none): the translator credits
 *
 * Sets the translator credits string which is displayed in
 * the translators tab of the secondary credits dialog.
 *
 * The intended use for this string is to display the translator
 * of the language which is currently used in the user interface.
 * Using gettext(), a simple way to achieve that is to mark the
 * string for translation:
 * |[
 *  btk_about_dialog_set_translator_credits (about, _("translator-credits"));
 * ]|
 * It is a good idea to use the customary msgid "translator-credits" for this
 * purpose, since translators will already know the purpose of that msgid, and
 * since #BtkAboutDialog will detect if "translator-credits" is untranslated
 * and hide the tab.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_translator_credits (BtkAboutDialog *about,
                                         const bchar    *translator_credits)
{
  BtkAboutDialogPrivate *priv;
  bchar *tmp;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  tmp = priv->translator_credits;
  priv->translator_credits = g_strdup (translator_credits);
  g_free (tmp);

  update_credits_button_visibility (about);

  g_object_notify (B_OBJECT (about), "translator-credits");
}

/**
 * btk_about_dialog_get_logo:
 * @about: a #BtkAboutDialog
 *
 * Returns the pixbuf displayed as logo in the about dialog.
 *
 * Return value: (transfer none): the pixbuf displayed as logo. The
 *   pixbuf is owned by the about dialog. If you want to keep a
 *   reference to it, you have to call g_object_ref() on it.
 *
 * Since: 2.6
 */
BdkPixbuf *
btk_about_dialog_get_logo (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  if (btk_image_get_storage_type (BTK_IMAGE (priv->logo_image)) == BTK_IMAGE_PIXBUF)
    return btk_image_get_pixbuf (BTK_IMAGE (priv->logo_image));
  else
    return NULL;
}

static BtkIconSet *
icon_set_new_from_pixbufs (GList *pixbufs)
{
  BtkIconSet *icon_set = btk_icon_set_new ();

  for (; pixbufs; pixbufs = pixbufs->next)
    {
      BdkPixbuf *pixbuf = BDK_PIXBUF (pixbufs->data);

      BtkIconSource *icon_source = btk_icon_source_new ();
      btk_icon_source_set_pixbuf (icon_source, pixbuf);
      btk_icon_set_add_source (icon_set, icon_source);
      btk_icon_source_free (icon_source);
    }

  return icon_set;
}

/**
 * btk_about_dialog_set_logo:
 * @about: a #BtkAboutDialog
 * @logo: (allow-none): a #BdkPixbuf, or %NULL
 *
 * Sets the pixbuf to be displayed as logo in the about dialog.
 * If it is %NULL, the default window icon set with
 * btk_window_set_default_icon() will be used.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_logo (BtkAboutDialog *about,
                           BdkPixbuf      *logo)
{
  BtkAboutDialogPrivate *priv;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  g_object_freeze_notify (B_OBJECT (about));

  if (btk_image_get_storage_type (BTK_IMAGE (priv->logo_image)) == BTK_IMAGE_ICON_NAME)
    g_object_notify (B_OBJECT (about), "logo-icon-name");

  if (logo != NULL)
    btk_image_set_from_pixbuf (BTK_IMAGE (priv->logo_image), logo);
  else
    {
      GList *pixbufs = btk_window_get_default_icon_list ();

      if (pixbufs != NULL)
        {
          BtkIconSet *icon_set = icon_set_new_from_pixbufs (pixbufs);

          btk_image_set_from_icon_set (BTK_IMAGE (priv->logo_image),
                                       icon_set, BTK_ICON_SIZE_DIALOG);

          btk_icon_set_unref (icon_set);
          g_list_free (pixbufs);
        }
    }

  g_object_notify (B_OBJECT (about), "logo");

  g_object_thaw_notify (B_OBJECT (about));
}

/**
 * btk_about_dialog_get_logo_icon_name:
 * @about: a #BtkAboutDialog
 *
 * Returns the icon name displayed as logo in the about dialog.
 *
 * Return value: the icon name displayed as logo. The string is
 *   owned by the dialog. If you want to keep a reference
 *   to it, you have to call g_strdup() on it.
 *
 * Since: 2.6
 */
const bchar *
btk_about_dialog_get_logo_icon_name (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv;
  const bchar *icon_name = NULL;

  g_return_val_if_fail (BTK_IS_ABOUT_DIALOG (about), NULL);

  priv = (BtkAboutDialogPrivate *)about->private_data;

  if (btk_image_get_storage_type (BTK_IMAGE (priv->logo_image)) == BTK_IMAGE_ICON_NAME)
    btk_image_get_icon_name (BTK_IMAGE (priv->logo_image), &icon_name, NULL);

  return icon_name;
}

/**
 * btk_about_dialog_set_logo_icon_name:
 * @about: a #BtkAboutDialog
 * @icon_name: (allow-none): an icon name, or %NULL
 *
 * Sets the pixbuf to be displayed as logo in the about dialog.
 * If it is %NULL, the default window icon set with
 * btk_window_set_default_icon() will be used.
 *
 * Since: 2.6
 */
void
btk_about_dialog_set_logo_icon_name (BtkAboutDialog *about,
                                     const bchar    *icon_name)
{
  BtkAboutDialogPrivate *priv;

  g_return_if_fail (BTK_IS_ABOUT_DIALOG (about));

  priv = (BtkAboutDialogPrivate *)about->private_data;

  g_object_freeze_notify (B_OBJECT (about));

  if (btk_image_get_storage_type (BTK_IMAGE (priv->logo_image)) == BTK_IMAGE_PIXBUF)
    g_object_notify (B_OBJECT (about), "logo");

  btk_image_set_from_icon_name (BTK_IMAGE (priv->logo_image), icon_name,
                                BTK_ICON_SIZE_DIALOG);
  g_object_notify (B_OBJECT (about), "logo-icon-name");

  g_object_thaw_notify (B_OBJECT (about));
}

static void
follow_if_link (BtkAboutDialog *about,
                BtkTextView    *text_view,
                BtkTextIter    *iter)
{
  GSList *tags = NULL, *tagp = NULL;
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;
  bchar *uri = NULL;

  tags = btk_text_iter_get_tags (iter);
  for (tagp = tags; tagp != NULL && !uri; tagp = tagp->next)
    {
      BtkTextTag *tag = tagp->data;

      uri = g_object_get_data (B_OBJECT (tag), "uri");
      if (uri)
        emit_activate_link (about, uri);

      if (uri && !b_slist_find_custom (priv->visited_links, uri, (GCompareFunc)strcmp))
        {
          BdkColor *style_visited_link_color;
          BdkColor color;

          btk_widget_ensure_style (BTK_WIDGET (about));
          btk_widget_style_get (BTK_WIDGET (about),
                                "visited-link-color", &style_visited_link_color,
                                NULL);
          if (style_visited_link_color)
            {
              color = *style_visited_link_color;
              bdk_color_free (style_visited_link_color);
            }
          else
            color = default_visited_link_color;

          g_object_set (B_OBJECT (tag), "foreground-bdk", &color, NULL);

          priv->visited_links = b_slist_prepend (priv->visited_links, g_strdup (uri));
        }
    }

  if (tags)
    b_slist_free (tags);
}

static bboolean
text_view_key_press_event (BtkWidget      *text_view,
                           BdkEventKey    *event,
                           BtkAboutDialog *about)
{
  BtkTextIter iter;
  BtkTextBuffer *buffer;

  switch (event->keyval)
    {
      case BDK_Return:
      case BDK_ISO_Enter:
      case BDK_KP_Enter:
        buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view));
        btk_text_buffer_get_iter_at_mark (buffer, &iter,
                                          btk_text_buffer_get_insert (buffer));
        follow_if_link (about, BTK_TEXT_VIEW (text_view), &iter);
        break;

      default:
        break;
    }

  return FALSE;
}

static bboolean
text_view_event_after (BtkWidget      *text_view,
                       BdkEvent       *event,
                       BtkAboutDialog *about)
{
  BtkTextIter start, end, iter;
  BtkTextBuffer *buffer;
  BdkEventButton *button_event;
  bint x, y;

  if (event->type != BDK_BUTTON_RELEASE)
    return FALSE;

  button_event = (BdkEventButton *)event;

  if (button_event->button != 1)
    return FALSE;

  buffer = btk_text_view_get_buffer (BTK_TEXT_VIEW (text_view));

  /* we shouldn't follow a link if the user has selected something */
  btk_text_buffer_get_selection_bounds (buffer, &start, &end);
  if (btk_text_iter_get_offset (&start) != btk_text_iter_get_offset (&end))
    return FALSE;

  btk_text_view_window_to_buffer_coords (BTK_TEXT_VIEW (text_view),
                                         BTK_TEXT_WINDOW_WIDGET,
                                         button_event->x, button_event->y, &x, &y);

  btk_text_view_get_iter_at_location (BTK_TEXT_VIEW (text_view), &iter, x, y);

  follow_if_link (about, BTK_TEXT_VIEW (text_view), &iter);

  return FALSE;
}

static void
set_cursor_if_appropriate (BtkAboutDialog *about,
                           BtkTextView    *text_view,
                           bint            x,
                           bint            y)
{
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;
  GSList *tags = NULL, *tagp = NULL;
  BtkTextIter iter;
  bboolean hovering_over_link = FALSE;

  btk_text_view_get_iter_at_location (text_view, &iter, x, y);

  tags = btk_text_iter_get_tags (&iter);
  for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
    {
      BtkTextTag *tag = tagp->data;
      bchar *uri = g_object_get_data (B_OBJECT (tag), "uri");

      if (uri != NULL)
        {
          hovering_over_link = TRUE;
          break;
        }
    }

  if (hovering_over_link != priv->hovering_over_link)
    {
      priv->hovering_over_link = hovering_over_link;

      if (hovering_over_link)
        bdk_window_set_cursor (btk_text_view_get_window (text_view, BTK_TEXT_WINDOW_TEXT), priv->hand_cursor);
      else
        bdk_window_set_cursor (btk_text_view_get_window (text_view, BTK_TEXT_WINDOW_TEXT), priv->regular_cursor);
    }

  if (tags)
    b_slist_free (tags);
}

static bboolean
text_view_motion_notify_event (BtkWidget *text_view,
                               BdkEventMotion *event,
                               BtkAboutDialog *about)
{
  bint x, y;

  btk_text_view_window_to_buffer_coords (BTK_TEXT_VIEW (text_view),
                                         BTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y, &x, &y);

  set_cursor_if_appropriate (about, BTK_TEXT_VIEW (text_view), x, y);

  bdk_event_request_motions (event);

  return FALSE;
}


static bboolean
text_view_visibility_notify_event (BtkWidget          *text_view,
                                   BdkEventVisibility *event,
                                   BtkAboutDialog     *about)
{
  bint wx, wy, bx, by;

  bdk_window_get_pointer (text_view->window, &wx, &wy, NULL);

  btk_text_view_window_to_buffer_coords (BTK_TEXT_VIEW (text_view),
                                         BTK_TEXT_WINDOW_WIDGET,
                                         wx, wy, &bx, &by);

  set_cursor_if_appropriate (about, BTK_TEXT_VIEW (text_view), bx, by);

  return FALSE;
}

static BtkWidget *
text_view_new (BtkAboutDialog  *about,
               BtkWidget       *dialog,
               bchar          **strings,
               BtkWrapMode      wrap_mode)
{
  bchar **p;
  bchar *q0, *q1, *q2, *r1, *r2;
  BtkWidget *view;
  BtkTextView *text_view;
  BtkTextBuffer *buffer;
  BdkColor *style_link_color;
  BdkColor *style_visited_link_color;
  BdkColor color;
  BdkColor link_color;
  BdkColor visited_link_color;
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;

  btk_widget_ensure_style (BTK_WIDGET (about));
  btk_widget_style_get (BTK_WIDGET (about),
                        "link-color", &style_link_color,
                        "visited-link-color", &style_visited_link_color,
                        NULL);
  if (style_link_color)
    {
      link_color = *style_link_color;
      bdk_color_free (style_link_color);
    }
  else
    link_color = default_link_color;

  if (style_visited_link_color)
    {
      visited_link_color = *style_visited_link_color;
      bdk_color_free (style_visited_link_color);
    }
  else
    visited_link_color = default_visited_link_color;

  view = btk_text_view_new ();
  text_view = BTK_TEXT_VIEW (view);
  buffer = btk_text_view_get_buffer (text_view);
  btk_text_view_set_cursor_visible (text_view, FALSE);
  btk_text_view_set_editable (text_view, FALSE);
  btk_text_view_set_wrap_mode (text_view, wrap_mode);

  btk_text_view_set_left_margin (text_view, 8);
  btk_text_view_set_right_margin (text_view, 8);

  g_signal_connect (view, "key-press-event",
                    G_CALLBACK (text_view_key_press_event), about);
  g_signal_connect (view, "event-after",
                    G_CALLBACK (text_view_event_after), about);
  g_signal_connect (view, "motion-notify-event",
                    G_CALLBACK (text_view_motion_notify_event), about);
  g_signal_connect (view, "visibility-notify-event",
                    G_CALLBACK (text_view_visibility_notify_event), about);

  if (strings == NULL)
    {
      btk_widget_hide (view);
      return view;
    }

  for (p = strings; *p; p++)
    {
      q0  = *p;
      while (*q0)
        {
          q1 = strchr (q0, '<');
          q2 = q1 ? strchr (q1, '>') : NULL;
          r1 = strstr (q0, "http://");
          if (r1)
            {
              r2 = strpbrk (r1, " \n\t");
              if (!r2)
                r2 = strchr (r1, '\0');
            }
          else
            r2 = NULL;

          if (r1 && r2 && (!q1 || !q2 || (r1 < q1)))
            {
              q1 = r1;
              q2 = r2;
            }

          if (q1 && q2)
            {
              BtkTextIter end;
              bchar *link;
              bchar *uri;
              const bchar *link_type;
              BtkTextTag *tag;

              if (*q1 == '<')
                {
                  btk_text_buffer_insert_at_cursor (buffer, q0, (q1 - q0) + 1);
                  btk_text_buffer_get_end_iter (buffer, &end);
                  q1++;
                  link_type = "email";
                }
              else
                {
                  btk_text_buffer_insert_at_cursor (buffer, q0, q1 - q0);
                  btk_text_buffer_get_end_iter (buffer, &end);
                  link_type = "uri";
                }

              q0 = q2;

              link = g_strndup (q1, q2 - q1);

              if (b_slist_find_custom (priv->visited_links, link, (GCompareFunc)strcmp))
                color = visited_link_color;
              else
                color = link_color;

              tag = btk_text_buffer_create_tag (buffer, NULL,
                                                "foreground-bdk", &color,
                                                "underline", BANGO_UNDERLINE_SINGLE,
                                                NULL);
              if (strcmp (link_type, "email") == 0)
                {
                  bchar *escaped;

                  escaped = g_uri_escape_string (link, NULL, FALSE);
                  uri = g_strconcat ("mailto:", escaped, NULL);
                  g_free (escaped);
                }
              else
                {
                  uri = g_strdup (link);
                }
              g_object_set_data_full (B_OBJECT (tag), I_("uri"), uri, g_free);
              btk_text_buffer_insert_with_tags (buffer, &end, link, -1, tag, NULL);

              g_free (link);
            }
          else
            {
              btk_text_buffer_insert_at_cursor (buffer, q0, -1);
              break;
            }
        }

      if (p[1])
        btk_text_buffer_insert_at_cursor (buffer, "\n", 1);
    }

  btk_widget_show (view);
  return view;
}

static void
add_credits_page (BtkAboutDialog *about,
                  BtkWidget      *credits_dialog,
                  BtkWidget      *notebook,
                  bchar          *title,
                  bchar         **people)
{
  BtkWidget *sw, *view;

  view = text_view_new (about, credits_dialog, people, BTK_WRAP_NONE);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
                                       BTK_SHADOW_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_AUTOMATIC,
                                  BTK_POLICY_AUTOMATIC);
  btk_container_add (BTK_CONTAINER (sw), view);

  btk_notebook_append_page (BTK_NOTEBOOK (notebook),
                            sw, btk_label_new (title));
}

static void
display_credits_dialog (BtkWidget *button,
                        bpointer   data)
{
  BtkAboutDialog *about = (BtkAboutDialog *)data;
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;
  BtkWidget *dialog, *notebook;
  BtkDialog *credits_dialog;

  if (priv->credits_dialog != NULL)
    {
      btk_window_present (BTK_WINDOW (priv->credits_dialog));
      return;
    }

  dialog = btk_dialog_new_with_buttons (_("Credits"),
                                        BTK_WINDOW (about),
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        BTK_STOCK_CLOSE, BTK_RESPONSE_CANCEL,
                                        NULL);
  credits_dialog = BTK_DIALOG (dialog);
  btk_dialog_set_has_separator (credits_dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (credits_dialog), 5);
  btk_box_set_spacing (BTK_BOX (credits_dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (credits_dialog->action_area), 5);

  priv->credits_dialog = dialog;
  btk_window_set_default_size (BTK_WINDOW (dialog), 360, 260);
  btk_dialog_set_default_response (credits_dialog, BTK_RESPONSE_CANCEL);

  btk_window_set_modal (BTK_WINDOW (dialog),
                        btk_window_get_modal (BTK_WINDOW (about)));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (btk_widget_destroy), dialog);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (btk_widget_destroyed),
                    &(priv->credits_dialog));

  notebook = btk_notebook_new ();
  btk_container_set_border_width (BTK_CONTAINER (notebook), 5);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), notebook, TRUE, TRUE, 0);

  if (priv->authors != NULL)
    add_credits_page (about, dialog, notebook, _("Written by"), priv->authors);

  if (priv->documenters != NULL)
    add_credits_page (about, dialog, notebook, _("Documented by"), priv->documenters);

  /* Don't show an untranslated gettext msgid */
  if (priv->translator_credits != NULL &&
      strcmp (priv->translator_credits, "translator_credits") != 0 &&
      strcmp (priv->translator_credits, "translator-credits") != 0)
    {
      bchar *translators[2];

      translators[0] = priv->translator_credits;
      translators[1] = NULL;

      add_credits_page (about, dialog, notebook, _("Translated by"), translators);
    }

  if (priv->artists != NULL)
    add_credits_page (about, dialog, notebook, _("Artwork by"), priv->artists);

  btk_widget_show_all (dialog);
}

static void
set_policy (BtkWidget *sw)
{
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_AUTOMATIC,
                                  BTK_POLICY_AUTOMATIC);
}

static void
display_license_dialog (BtkWidget *button,
                        bpointer   data)
{
  BtkAboutDialog *about = (BtkAboutDialog *)data;
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;
  BtkWidget *dialog, *view, *sw;
  BtkDialog *licence_dialog;
  bchar *strings[2];

  if (priv->license_dialog != NULL)
    {
      btk_window_present (BTK_WINDOW (priv->license_dialog));
      return;
    }

  dialog = btk_dialog_new_with_buttons (_("License"),
                                        BTK_WINDOW (about),
                                        BTK_DIALOG_DESTROY_WITH_PARENT,
                                        BTK_STOCK_CLOSE, BTK_RESPONSE_CANCEL,
                                        NULL);
  licence_dialog = BTK_DIALOG (dialog);
  btk_dialog_set_has_separator (licence_dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (licence_dialog), 5);
  btk_box_set_spacing (BTK_BOX (licence_dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (licence_dialog->action_area), 5);

  priv->license_dialog = dialog;
  btk_window_set_default_size (BTK_WINDOW (dialog), 420, 320);
  btk_dialog_set_default_response (licence_dialog, BTK_RESPONSE_CANCEL);

  btk_window_set_modal (BTK_WINDOW (dialog),
                        btk_window_get_modal (BTK_WINDOW (about)));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (btk_widget_destroy), dialog);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (btk_widget_destroyed),
                    &(priv->license_dialog));

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_container_set_border_width (BTK_CONTAINER (sw), 5);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
                                       BTK_SHADOW_IN);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (sw),
                                  BTK_POLICY_NEVER,
                                  BTK_POLICY_AUTOMATIC);
  g_signal_connect (sw, "map", G_CALLBACK (set_policy), NULL);
  btk_box_pack_start (BTK_BOX (BTK_DIALOG (dialog)->vbox), sw, TRUE, TRUE, 0);

  strings[0] = priv->license;
  strings[1] = NULL;
  view = text_view_new (about, dialog, strings,
                        priv->wrap_license ? BTK_WRAP_WORD : BTK_WRAP_NONE);

  btk_container_add (BTK_CONTAINER (sw), view);

  btk_widget_show_all (dialog);
}

/**
 * btk_about_dialog_new:
 *
 * Creates a new #BtkAboutDialog.
 *
 * Returns: a newly created #BtkAboutDialog
 *
 * Since: 2.6
 */
BtkWidget *
btk_about_dialog_new (void)
{
  BtkAboutDialog *dialog = g_object_new (BTK_TYPE_ABOUT_DIALOG, NULL);

  return BTK_WIDGET (dialog);
}

/**
 * btk_about_dialog_set_email_hook:
 * @func: a function to call when an email link is activated.
 * @data: data to pass to @func
 * @destroy: #GDestroyNotify for @data
 *
 * Installs a global function to be called whenever the user activates an
 * email link in an about dialog.
 *
 * Since 2.18 there exists a default function which uses btk_show_uri(). To
 * deactivate it, you can pass %NULL for @func.
 *
 * Return value: the previous email hook.
 *
 * Since: 2.6
 *
 * Deprecated: 2.24: Use the #BtkAboutDialog::activate-link signal
 */
BtkAboutDialogActivateLinkFunc
btk_about_dialog_set_email_hook (BtkAboutDialogActivateLinkFunc func,
                                 bpointer                       data,
                                 GDestroyNotify                 destroy)
{
  BtkAboutDialogActivateLinkFunc old;

  if (activate_email_hook_destroy != NULL)
    (* activate_email_hook_destroy) (activate_email_hook_data);

  old = activate_email_hook;

  activate_email_hook_set = TRUE;
  activate_email_hook = func;
  activate_email_hook_data = data;
  activate_email_hook_destroy = destroy;

  return old;
}

/**
 * btk_about_dialog_set_url_hook:
 * @func: a function to call when a URL link is activated.
 * @data: data to pass to @func
 * @destroy: #GDestroyNotify for @data
 *
 * Installs a global function to be called whenever the user activates a
 * URL link in an about dialog.
 *
 * Since 2.18 there exists a default function which uses btk_show_uri(). To
 * deactivate it, you can pass %NULL for @func.
 *
 * Return value: the previous URL hook.
 *
 * Since: 2.6
 *
 * Deprecated: 2.24: Use the #BtkAboutDialog::activate-link signal
 */
BtkAboutDialogActivateLinkFunc
btk_about_dialog_set_url_hook (BtkAboutDialogActivateLinkFunc func,
                               bpointer                       data,
                               GDestroyNotify                 destroy)
{
  BtkAboutDialogActivateLinkFunc old;

  if (activate_url_hook_destroy != NULL)
    (* activate_url_hook_destroy) (activate_url_hook_data);

  old = activate_url_hook;

  activate_url_hook_set = TRUE;
  activate_url_hook = func;
  activate_url_hook_data = data;
  activate_url_hook_destroy = destroy;

  return old;
}

static void
close_cb (BtkAboutDialog *about)
{
  BtkAboutDialogPrivate *priv = (BtkAboutDialogPrivate *)about->private_data;

  if (priv->license_dialog != NULL)
    {
      btk_widget_destroy (priv->license_dialog);
      priv->license_dialog = NULL;
    }

  if (priv->credits_dialog != NULL)
    {
      btk_widget_destroy (priv->credits_dialog);
      priv->credits_dialog = NULL;
    }

  btk_widget_hide (BTK_WIDGET (about));

}

/**
 * btk_show_about_dialog:
 * @parent: (allow-none): transient parent, or %NULL for none
 * @first_property_name: the name of the first property
 * @Varargs: value of first property, followed by more properties, %NULL-terminated
 *
 * This is a convenience function for showing an application's about box.
 * The constructed dialog is associated with the parent window and
 * reused for future invocations of this function.
 *
 * Since: 2.6
 */
void
btk_show_about_dialog (BtkWindow   *parent,
                       const bchar *first_property_name,
                       ...)
{
  static BtkWidget *global_about_dialog = NULL;
  BtkWidget *dialog = NULL;
  va_list var_args;

  if (parent)
    dialog = g_object_get_data (B_OBJECT (parent), "btk-about-dialog");
  else
    dialog = global_about_dialog;

  if (!dialog)
    {
      dialog = btk_about_dialog_new ();

      g_object_ref_sink (dialog);

      g_signal_connect (dialog, "delete-event",
                        G_CALLBACK (btk_widget_hide_on_delete), NULL);

      /* Close dialog on user response */
      g_signal_connect (dialog, "response",
                        G_CALLBACK (close_cb), NULL);

      va_start (var_args, first_property_name);
      g_object_set_valist (B_OBJECT (dialog), first_property_name, var_args);
      va_end (var_args);

      if (parent)
        {
          btk_window_set_transient_for (BTK_WINDOW (dialog), parent);
          btk_window_set_destroy_with_parent (BTK_WINDOW (dialog), TRUE);
          g_object_set_data_full (B_OBJECT (parent),
                                  I_("btk-about-dialog"),
                                  dialog, g_object_unref);
        }
      else
        global_about_dialog = dialog;

    }

  btk_window_present (BTK_WINDOW (dialog));
}

#define __BTK_ABOUT_DIALOG_C__
#include "btkaliasdef.c"
