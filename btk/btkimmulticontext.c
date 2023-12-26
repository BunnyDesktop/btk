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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <locale.h>

#include "btkimmulticontext.h"
#include "btkimmodule.h"
#include "btkmain.h"
#include "btkradiomenuitem.h"
#include "btkintl.h"
#include "btkprivate.h" /* To get redefinition of BTK_LOCALE_DIR on Win32 */
#include "btkalias.h"

#define NONE_ID "btk-im-context-none"

struct _BtkIMMulticontextPrivate
{
  BdkWindow *client_window;
  BdkRectangle cursor_location;
  bchar *context_id;

  buint use_preedit : 1;
  buint have_cursor_location : 1;
  buint focus_in : 1;
};

static void     btk_im_multicontext_finalize           (BObject                 *object);

static void     btk_im_multicontext_set_slave          (BtkIMMulticontext       *multicontext,
							BtkIMContext            *slave,
							bboolean                 finalizing);

static void     btk_im_multicontext_set_client_window  (BtkIMContext            *context,
							BdkWindow               *window);
static void     btk_im_multicontext_get_preedit_string (BtkIMContext            *context,
							bchar                  **str,
							BangoAttrList          **attrs,
							bint                   *cursor_pos);
static bboolean btk_im_multicontext_filter_keypress    (BtkIMContext            *context,
							BdkEventKey             *event);
static void     btk_im_multicontext_focus_in           (BtkIMContext            *context);
static void     btk_im_multicontext_focus_out          (BtkIMContext            *context);
static void     btk_im_multicontext_reset              (BtkIMContext            *context);
static void     btk_im_multicontext_set_cursor_location (BtkIMContext            *context,
							BdkRectangle		*area);
static void     btk_im_multicontext_set_use_preedit    (BtkIMContext            *context,
							bboolean                 use_preedit);
static bboolean btk_im_multicontext_get_surrounding    (BtkIMContext            *context,
							bchar                  **text,
							bint                    *cursor_index);
static void     btk_im_multicontext_set_surrounding    (BtkIMContext            *context,
							const char              *text,
							bint                     len,
							bint                     cursor_index);

static void     btk_im_multicontext_preedit_start_cb        (BtkIMContext      *slave,
							     BtkIMMulticontext *multicontext);
static void     btk_im_multicontext_preedit_end_cb          (BtkIMContext      *slave,
							     BtkIMMulticontext *multicontext);
static void     btk_im_multicontext_preedit_changed_cb      (BtkIMContext      *slave,
							     BtkIMMulticontext *multicontext);
static void     btk_im_multicontext_commit_cb               (BtkIMContext      *slave,
							     const bchar       *str,
							     BtkIMMulticontext *multicontext);
static bboolean btk_im_multicontext_retrieve_surrounding_cb (BtkIMContext      *slave,
							     BtkIMMulticontext *multicontext);
static bboolean btk_im_multicontext_delete_surrounding_cb   (BtkIMContext      *slave,
							     bint               offset,
							     bint               n_chars,
							     BtkIMMulticontext *multicontext);

static const bchar *global_context_id = NULL;

G_DEFINE_TYPE (BtkIMMulticontext, btk_im_multicontext, BTK_TYPE_IM_CONTEXT)

static void
btk_im_multicontext_class_init (BtkIMMulticontextClass *class)
{
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);
  BtkIMContextClass *im_context_class = BTK_IM_CONTEXT_CLASS (class);
  
  im_context_class->set_client_window = btk_im_multicontext_set_client_window;
  im_context_class->get_preedit_string = btk_im_multicontext_get_preedit_string;
  im_context_class->filter_keypress = btk_im_multicontext_filter_keypress;
  im_context_class->focus_in = btk_im_multicontext_focus_in;
  im_context_class->focus_out = btk_im_multicontext_focus_out;
  im_context_class->reset = btk_im_multicontext_reset;
  im_context_class->set_cursor_location = btk_im_multicontext_set_cursor_location;
  im_context_class->set_use_preedit = btk_im_multicontext_set_use_preedit;
  im_context_class->set_surrounding = btk_im_multicontext_set_surrounding;
  im_context_class->get_surrounding = btk_im_multicontext_get_surrounding;

  bobject_class->finalize = btk_im_multicontext_finalize;

  g_type_class_add_private (bobject_class, sizeof (BtkIMMulticontextPrivate));   
}

static void
btk_im_multicontext_init (BtkIMMulticontext *multicontext)
{
  multicontext->slave = NULL;
  
  multicontext->priv = B_TYPE_INSTANCE_GET_PRIVATE (multicontext, BTK_TYPE_IM_MULTICONTEXT, BtkIMMulticontextPrivate);
  multicontext->priv->use_preedit = TRUE;
  multicontext->priv->have_cursor_location = FALSE;
  multicontext->priv->focus_in = FALSE;
}

/**
 * btk_im_multicontext_new:
 *
 * Creates a new #BtkIMMulticontext.
 *
 * Returns: a new #BtkIMMulticontext.
 **/
BtkIMContext *
btk_im_multicontext_new (void)
{
  return g_object_new (BTK_TYPE_IM_MULTICONTEXT, NULL);
}

static void
btk_im_multicontext_finalize (BObject *object)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (object);
  
  btk_im_multicontext_set_slave (multicontext, NULL, TRUE);
  g_free (multicontext->context_id);
  g_free (multicontext->priv->context_id);

  B_OBJECT_CLASS (btk_im_multicontext_parent_class)->finalize (object);
}

static void
btk_im_multicontext_set_slave (BtkIMMulticontext *multicontext,
			       BtkIMContext      *slave,
			       bboolean           finalizing)
{
  BtkIMMulticontextPrivate *priv = multicontext->priv;
  bboolean need_preedit_changed = FALSE;
  
  if (multicontext->slave)
    {
      if (!finalizing)
	btk_im_context_reset (multicontext->slave);
      
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    btk_im_multicontext_preedit_start_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    btk_im_multicontext_preedit_end_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    btk_im_multicontext_preedit_changed_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (multicontext->slave,
					    btk_im_multicontext_commit_cb,
					    multicontext);

      g_object_unref (multicontext->slave);
      multicontext->slave = NULL;

      if (!finalizing)
	need_preedit_changed = TRUE;
    }
  
  multicontext->slave = slave;

  if (multicontext->slave)
    {
      g_object_ref (multicontext->slave);

      g_signal_connect (multicontext->slave, "preedit-start",
			G_CALLBACK (btk_im_multicontext_preedit_start_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "preedit-end",
			G_CALLBACK (btk_im_multicontext_preedit_end_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "preedit-changed",
			G_CALLBACK (btk_im_multicontext_preedit_changed_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "commit",
			G_CALLBACK (btk_im_multicontext_commit_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "retrieve-surrounding",
			G_CALLBACK (btk_im_multicontext_retrieve_surrounding_cb),
			multicontext);
      g_signal_connect (multicontext->slave, "delete-surrounding",
			G_CALLBACK (btk_im_multicontext_delete_surrounding_cb),
			multicontext);
      
      if (!priv->use_preedit)	/* Default is TRUE */
	btk_im_context_set_use_preedit (slave, FALSE);
      if (priv->client_window)
	btk_im_context_set_client_window (slave, priv->client_window);
      if (priv->have_cursor_location)
	btk_im_context_set_cursor_location (slave, &priv->cursor_location);
      if (priv->focus_in)
	btk_im_context_focus_in (slave);
    }

  if (need_preedit_changed)
    g_signal_emit_by_name (multicontext, "preedit-changed");
}

static const bchar *
get_effective_context_id (BtkIMMulticontext *multicontext)
{
  if (multicontext->priv->context_id)
    return multicontext->priv->context_id;

  if (!global_context_id)
    global_context_id = _btk_im_module_get_default_context_id (multicontext->priv->client_window);

  return global_context_id;
}

static BtkIMContext *
btk_im_multicontext_get_slave (BtkIMMulticontext *multicontext)
{
  if (g_strcmp0 (multicontext->context_id, get_effective_context_id (multicontext)) != 0)
    btk_im_multicontext_set_slave (multicontext, NULL, FALSE);

  if (!multicontext->slave)
    {
      BtkIMContext *slave;

      g_free (multicontext->context_id);

      multicontext->context_id = g_strdup (get_effective_context_id (multicontext));

      if (g_strcmp0 (multicontext->context_id, NONE_ID) == 0)
        return NULL;

      slave = _btk_im_module_create (multicontext->context_id);
      btk_im_multicontext_set_slave (multicontext, slave, FALSE);
      g_object_unref (slave);
    }

  return multicontext->slave;
}

static void
im_module_setting_changed (BtkSettings *settings, 
                           bpointer     data)
{
  global_context_id = NULL;
}


static void
btk_im_multicontext_set_client_window (BtkIMContext *context,
				       BdkWindow    *window)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave;
  BdkScreen *screen;
  BtkSettings *settings;
  bboolean connected;

  multicontext->priv->client_window = window;

  if (window)
    {
      screen = bdk_window_get_screen (window);
      settings = btk_settings_get_for_screen (screen);

      connected = BPOINTER_TO_INT (g_object_get_data (B_OBJECT (settings),
                                                      "btk-im-module-connected"));
      if (!connected)
        {
          g_signal_connect (settings, "notify::btk-im-module",
                            G_CALLBACK (im_module_setting_changed), NULL);
          g_object_set_data (B_OBJECT (settings), "btk-im-module-connected",
                             BINT_TO_POINTER (TRUE));

          global_context_id = NULL;
        }
    }

  slave = btk_im_multicontext_get_slave (multicontext);
  if (slave)
    btk_im_context_set_client_window (slave, window);
}

static void
btk_im_multicontext_get_preedit_string (BtkIMContext   *context,
					bchar         **str,
					BangoAttrList **attrs,
					bint           *cursor_pos)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  if (slave)
    btk_im_context_get_preedit_string (slave, str, attrs, cursor_pos);
  else
    {
      if (str)
	*str = g_strdup ("");
      if (attrs)
	*attrs = bango_attr_list_new ();
    }
}

static bboolean
btk_im_multicontext_filter_keypress (BtkIMContext *context,
				     BdkEventKey  *event)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  if (slave)
    return btk_im_context_filter_keypress (slave, event);
  else if (event->type == BDK_KEY_PRESS &&
           (event->state & BTK_NO_TEXT_INPUT_MOD_MASK) == 0)
    {
      gunichar ch;

      ch = bdk_keyval_to_unicode (event->keyval);
      if (ch != 0)
        {
          bint len;
          bchar buf[10];

          len = g_unichar_to_utf8 (ch, buf);
          buf[len] = '\0';

          g_signal_emit_by_name (multicontext, "commit", buf);

          return TRUE;
        }
    }

  return FALSE;
}

static void
btk_im_multicontext_focus_in (BtkIMContext   *context)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  multicontext->priv->focus_in = TRUE;

  if (slave)
    btk_im_context_focus_in (slave);
}

static void
btk_im_multicontext_focus_out (BtkIMContext   *context)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  multicontext->priv->focus_in = FALSE;
  
  if (slave)
    btk_im_context_focus_out (slave);
}

static void
btk_im_multicontext_reset (BtkIMContext   *context)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  if (slave)
    btk_im_context_reset (slave);
}

static void
btk_im_multicontext_set_cursor_location (BtkIMContext   *context,
					 BdkRectangle   *area)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  multicontext->priv->have_cursor_location = TRUE;
  multicontext->priv->cursor_location = *area;

  if (slave)
    btk_im_context_set_cursor_location (slave, area);
}

static void
btk_im_multicontext_set_use_preedit (BtkIMContext   *context,
				     bboolean	    use_preedit)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  use_preedit = use_preedit != FALSE;

  multicontext->priv->use_preedit = use_preedit;

  if (slave)
    btk_im_context_set_use_preedit (slave, use_preedit);
}

static bboolean
btk_im_multicontext_get_surrounding (BtkIMContext  *context,
				     bchar        **text,
				     bint          *cursor_index)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  if (slave)
    return btk_im_context_get_surrounding (slave, text, cursor_index);
  else
    {
      if (text)
	*text = NULL;
      if (cursor_index)
	*cursor_index = 0;

      return FALSE;
    }
}

static void
btk_im_multicontext_set_surrounding (BtkIMContext *context,
				     const char   *text,
				     bint          len,
				     bint          cursor_index)
{
  BtkIMMulticontext *multicontext = BTK_IM_MULTICONTEXT (context);
  BtkIMContext *slave = btk_im_multicontext_get_slave (multicontext);

  if (slave)
    btk_im_context_set_surrounding (slave, text, len, cursor_index);
}

static void
btk_im_multicontext_preedit_start_cb   (BtkIMContext      *slave,
					BtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-start");
}

static void
btk_im_multicontext_preedit_end_cb (BtkIMContext      *slave,
				    BtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-end");
}

static void
btk_im_multicontext_preedit_changed_cb (BtkIMContext      *slave,
					BtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-changed");
}

static void
btk_im_multicontext_commit_cb (BtkIMContext      *slave,
			       const bchar       *str,
			       BtkIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "commit", str);
}

static bboolean
btk_im_multicontext_retrieve_surrounding_cb (BtkIMContext      *slave,
					     BtkIMMulticontext *multicontext)
{
  bboolean result;
  
  g_signal_emit_by_name (multicontext, "retrieve-surrounding", &result);

  return result;
}

static bboolean
btk_im_multicontext_delete_surrounding_cb (BtkIMContext      *slave,
					   bint               offset,
					   bint               n_chars,
					   BtkIMMulticontext *multicontext)
{
  bboolean result;
  
  g_signal_emit_by_name (multicontext, "delete-surrounding",
			 offset, n_chars, &result);

  return result;
}

static void
activate_cb (BtkWidget         *menuitem,
	     BtkIMMulticontext *context)
{
  if (BTK_CHECK_MENU_ITEM (menuitem)->active)
    {
      const bchar *id = g_object_get_data (B_OBJECT (menuitem), "btk-context-id");

      btk_im_multicontext_set_context_id (context, id);
    }
}

static int
pathnamecmp (const char *a,
	     const char *b)
{
#ifndef G_OS_WIN32
  return strcmp (a, b);
#else
  /* Ignore case insensitivity, probably not that relevant here. Just
   * make sure slash and backslash compare equal.
   */
  while (*a && *b)
    if ((G_IS_DIR_SEPARATOR (*a) && G_IS_DIR_SEPARATOR (*b)) ||
	*a == *b)
      a++, b++;
    else
      return (*a - *b);
  return (*a - *b);
#endif
}

/**
 * btk_im_multicontext_append_menuitems:
 * @context: a #BtkIMMulticontext
 * @menushell: a #BtkMenuShell
 * 
 * Add menuitems for various available input methods to a menu;
 * the menuitems, when selected, will switch the input method
 * for the context and the global default input method.
 **/
void
btk_im_multicontext_append_menuitems (BtkIMMulticontext *context,
				      BtkMenuShell      *menushell)
{
  const BtkIMContextInfo **contexts;
  buint n_contexts, i;
  GSList *group = NULL;
  BtkWidget *menuitem, *system_menuitem;
  const char *system_context_id; 
  
  system_context_id = _btk_im_module_get_default_context_id (context->priv->client_window);
  system_menuitem = menuitem = btk_radio_menu_item_new_with_label (group, C_("input method menu", "System"));
  if (!context->priv->context_id)
    btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (menuitem), TRUE);
  group = btk_radio_menu_item_get_group (BTK_RADIO_MENU_ITEM (menuitem));
  g_object_set_data (B_OBJECT (menuitem), I_("btk-context-id"), NULL);
  g_signal_connect (menuitem, "activate", G_CALLBACK (activate_cb), context);

  btk_widget_show (menuitem);
  btk_menu_shell_append (menushell, menuitem);

  menuitem = btk_radio_menu_item_new_with_label (group, C_("input method menu", "None"));
  if (g_strcmp0 (context->priv->context_id, NONE_ID) == 0)
    btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (menuitem), TRUE);
  g_object_set_data (B_OBJECT (menuitem), I_("btk-context-id"), NONE_ID);
  g_signal_connect (menuitem, "activate", G_CALLBACK (activate_cb), context);
  btk_widget_show (menuitem);
  btk_menu_shell_append (menushell, menuitem);
  group = btk_radio_menu_item_get_group (BTK_RADIO_MENU_ITEM (menuitem));
  
  menuitem = btk_separator_menu_item_new ();
  btk_widget_show (menuitem);
  btk_menu_shell_append (menushell, menuitem);

  _btk_im_module_list (&contexts, &n_contexts);

  for (i = 0; i < n_contexts; i++)
    {
      const bchar *translated_name;
#ifdef ENABLE_NLS
      if (contexts[i]->domain && contexts[i]->domain[0])
	{
	  if (strcmp (contexts[i]->domain, GETTEXT_PACKAGE) == 0)
	    {
	      /* Same translation domain as BTK+ */
	      if (!(contexts[i]->domain_dirname && contexts[i]->domain_dirname[0]) ||
		  pathnamecmp (contexts[i]->domain_dirname, BTK_LOCALEDIR) == 0)
		{
		  /* Empty or NULL, domain directory, or same as
		   * BTK+. Input method may have a name in the BTK+
		   * message catalog.
		   */
		  translated_name = _(contexts[i]->context_name);
		}
	      else
		{
		  /* Separate domain directory but the same
		   * translation domain as BTK+. We can't call
		   * bindtextdomain() as that would make BTK+ forget
		   * its own messages.
		   */
		  g_warning ("Input method %s should not use BTK's translation domain %s",
			     contexts[i]->context_id, GETTEXT_PACKAGE);
		  /* Try translating the name in BTK+'s domain */
		  translated_name = _(contexts[i]->context_name);
		}
	    }
	  else if (contexts[i]->domain_dirname && contexts[i]->domain_dirname[0])
	    /* Input method has own translation domain and message catalog */
	    {
	      bindtextdomain (contexts[i]->domain,
			      contexts[i]->domain_dirname);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	      bind_textdomain_codeset (contexts[i]->domain, "UTF-8");
#endif
	      translated_name = g_dgettext (contexts[i]->domain, contexts[i]->context_name);
	    }
	  else
	    {
	      /* Different translation domain, but no domain directory */
	      translated_name = contexts[i]->context_name;
	    }
	}
      else
	/* Empty or NULL domain. We assume that input method does not
	 * want a translated name in this case.
	 */
	translated_name = contexts[i]->context_name;
#else
      translated_name = contexts[i]->context_name;
#endif
      menuitem = btk_radio_menu_item_new_with_label (group,
						     translated_name);
      
      if ((context->priv->context_id &&
           strcmp (contexts[i]->context_id, context->priv->context_id) == 0))
        btk_check_menu_item_set_active (BTK_CHECK_MENU_ITEM (menuitem), TRUE);

      if (strcmp (contexts[i]->context_id, system_context_id) == 0)
        {
          BtkWidget *label;
          char *text;

          label = btk_bin_get_child (BTK_BIN (system_menuitem));
          text = g_strdup_printf (C_("input method menu", "System (%s)"), translated_name);
          btk_label_set_text (BTK_LABEL (label), text);
          g_free (text);
        }     
 
      group = btk_radio_menu_item_get_group (BTK_RADIO_MENU_ITEM (menuitem));
      
      g_object_set_data (B_OBJECT (menuitem), I_("btk-context-id"),
			 (char *)contexts[i]->context_id);
      g_signal_connect (menuitem, "activate",
			G_CALLBACK (activate_cb), context);

      btk_widget_show (menuitem);
      btk_menu_shell_append (menushell, menuitem);
    }

  g_free (contexts);
}

/**
 * btk_im_multicontext_get_context_id:
 * @context: a #BtkIMMulticontext
 *
 * Gets the id of the currently active slave of the @context.
 *
 * Returns: the id of the currently active slave
 *
 * Since: 2.16
 */
const char *
btk_im_multicontext_get_context_id (BtkIMMulticontext *context)
{
  return context->context_id;
}

/**
 * btk_im_multicontext_set_context_id:
 * @context: a #BtkIMMulticontext
 * @context_id: the id to use 
 *
 * Sets the context id for @context.
 *
 * This causes the currently active slave of @context to be
 * replaced by the slave corresponding to the new context id.
 *
 * Since: 2.16
 */
void
btk_im_multicontext_set_context_id (BtkIMMulticontext *context,
                                    const char        *context_id)
{
  btk_im_context_reset (BTK_IM_CONTEXT (context));
  g_free (context->priv->context_id);
  context->priv->context_id = g_strdup (context_id);
  btk_im_multicontext_set_slave (context, NULL, FALSE);
}


#define __BTK_IM_MULTICONTEXT_C__
#include "btkaliasdef.c"
