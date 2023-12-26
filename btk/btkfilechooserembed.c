/* BTK - The GIMP Toolkit
 * btkfilechooserembed.h: Abstract sizing interface for file selector implementations
 * Copyright (C) 2004, Red Hat, Inc.
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
#include "btkfilechooserembed.h"
#include "btkmarshalers.h"
#include "btkintl.h"
#include "btkalias.h"

static void btk_file_chooser_embed_class_init (gpointer g_iface);
static void delegate_get_default_size         (BtkFileChooserEmbed *chooser_embed,
					       gint                *default_width,
					       gint                *default_height);
static gboolean delegate_should_respond       (BtkFileChooserEmbed *chooser_embed);
static void delegate_initial_focus            (BtkFileChooserEmbed *chooser_embed);
static void delegate_default_size_changed     (BtkFileChooserEmbed *chooser_embed,
					       gpointer             data);
static void delegate_response_requested       (BtkFileChooserEmbed *chooser_embed,
					       gpointer             data);

static BtkFileChooserEmbed *
get_delegate (BtkFileChooserEmbed *receiver)
{
  return g_object_get_data (G_OBJECT (receiver), "btk-file-chooser-embed-delegate");
}

/**
 * _btk_file_chooser_embed_delegate_iface_init:
 * @iface: a #BtkFileChoserEmbedIface structure
 * 
 * An interface-initialization function for use in cases where an object is
 * simply delegating the methods, signals of the #BtkFileChooserEmbed interface
 * to another object.  _btk_file_chooser_embed_set_delegate() must be called on
 * each instance of the object so that the delegate object can be found.
 **/
void
_btk_file_chooser_embed_delegate_iface_init (BtkFileChooserEmbedIface *iface)
{
  iface->get_default_size = delegate_get_default_size;
  iface->should_respond = delegate_should_respond;
  iface->initial_focus = delegate_initial_focus;
}

/**
 * _btk_file_chooser_embed_set_delegate:
 * @receiver: a GOobject implementing #BtkFileChooserEmbed
 * @delegate: another GObject implementing #BtkFileChooserEmbed
 *
 * Establishes that calls on @receiver for #BtkFileChooser methods should be
 * delegated to @delegate, and that #BtkFileChooser signals emitted on @delegate
 * should be forwarded to @receiver. Must be used in conjunction with
 * _btk_file_chooser_embed_delegate_iface_init().
 **/
void
_btk_file_chooser_embed_set_delegate (BtkFileChooserEmbed *receiver,
				      BtkFileChooserEmbed *delegate)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_EMBED (receiver));
  g_return_if_fail (BTK_IS_FILE_CHOOSER_EMBED (delegate));
  
  g_object_set_data (G_OBJECT (receiver), I_("btk-file-chooser-embed-delegate"), delegate);

  g_signal_connect (delegate, "default-size-changed",
		    G_CALLBACK (delegate_default_size_changed), receiver);
  g_signal_connect (delegate, "response-requested",
		    G_CALLBACK (delegate_response_requested), receiver);
}



static void
delegate_get_default_size (BtkFileChooserEmbed *chooser_embed,
			   gint                *default_width,
			   gint                *default_height)
{
  _btk_file_chooser_embed_get_default_size (get_delegate (chooser_embed), default_width, default_height);
}

static gboolean
delegate_should_respond (BtkFileChooserEmbed *chooser_embed)
{
  return _btk_file_chooser_embed_should_respond (get_delegate (chooser_embed));
}

static void
delegate_initial_focus (BtkFileChooserEmbed *chooser_embed)
{
  _btk_file_chooser_embed_initial_focus (get_delegate (chooser_embed));
}

static void
delegate_default_size_changed (BtkFileChooserEmbed *chooser_embed,
			       gpointer             data)
{
  g_signal_emit_by_name (data, "default-size-changed");
}

static void
delegate_response_requested (BtkFileChooserEmbed *chooser_embed,
			     gpointer             data)
{
  g_signal_emit_by_name (data, "response-requested");
}


/* publicly callable functions */

GType
_btk_file_chooser_embed_get_type (void)
{
  static GType file_chooser_embed_type = 0;

  if (!file_chooser_embed_type)
    {
      const GTypeInfo file_chooser_embed_info =
      {
	sizeof (BtkFileChooserEmbedIface),  /* class_size */
	NULL,                          /* base_init */
	NULL,			       /* base_finalize */
	(GClassInitFunc)btk_file_chooser_embed_class_init, /* class_init */
      };

      file_chooser_embed_type = g_type_register_static (G_TYPE_INTERFACE,
							I_("BtkFileChooserEmbed"),
							&file_chooser_embed_info, 0);

      g_type_interface_add_prerequisite (file_chooser_embed_type, BTK_TYPE_WIDGET);
    }

  return file_chooser_embed_type;
}

static void
btk_file_chooser_embed_class_init (gpointer g_iface)
{
  GType iface_type = G_TYPE_FROM_INTERFACE (g_iface);

  g_signal_new (I_("default-size-changed"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserEmbedIface, default_size_changed),
		NULL, NULL,
		_btk_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
  g_signal_new (I_("response-requested"),
		iface_type,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BtkFileChooserEmbedIface, response_requested),
		NULL, NULL,
		_btk_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

void
_btk_file_chooser_embed_get_default_size (BtkFileChooserEmbed *chooser_embed,
					 gint                *default_width,
					 gint                *default_height)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_EMBED (chooser_embed));
  g_return_if_fail (default_width != NULL);
  g_return_if_fail (default_height != NULL);

  BTK_FILE_CHOOSER_EMBED_GET_IFACE (chooser_embed)->get_default_size (chooser_embed, default_width, default_height);
}

gboolean
_btk_file_chooser_embed_should_respond (BtkFileChooserEmbed *chooser_embed)
{
  g_return_val_if_fail (BTK_IS_FILE_CHOOSER_EMBED (chooser_embed), FALSE);

  return BTK_FILE_CHOOSER_EMBED_GET_IFACE (chooser_embed)->should_respond (chooser_embed);
}

void
_btk_file_chooser_embed_initial_focus (BtkFileChooserEmbed *chooser_embed)
{
  g_return_if_fail (BTK_IS_FILE_CHOOSER_EMBED (chooser_embed));

  BTK_FILE_CHOOSER_EMBED_GET_IFACE (chooser_embed)->initial_focus (chooser_embed);
}
