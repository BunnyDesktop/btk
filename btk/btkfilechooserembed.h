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

#ifndef __BTK_FILE_CHOOSER_EMBED_H__
#define __BTK_FILE_CHOOSER_EMBED_H__

#include <btk/btkwidget.h>

B_BEGIN_DECLS

#define BTK_TYPE_FILE_CHOOSER_EMBED             (_btk_file_chooser_embed_get_type ())
#define BTK_FILE_CHOOSER_EMBED(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_FILE_CHOOSER_EMBED, BtkFileChooserEmbed))
#define BTK_IS_FILE_CHOOSER_EMBED(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_FILE_CHOOSER_EMBED))
#define BTK_FILE_CHOOSER_EMBED_GET_IFACE(obj)   (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BTK_TYPE_FILE_CHOOSER_EMBED, BtkFileChooserEmbedIface))

typedef struct _BtkFileChooserEmbed      BtkFileChooserEmbed;
typedef struct _BtkFileChooserEmbedIface BtkFileChooserEmbedIface;


struct _BtkFileChooserEmbedIface
{
  GTypeInterface base_iface;

  /* Methods
   */
  void (*get_default_size)        (BtkFileChooserEmbed *chooser_embed,
				   gint                *default_width,
				   gint                *default_height);

  gboolean (*should_respond)      (BtkFileChooserEmbed *chooser_embed);

  void (*initial_focus)           (BtkFileChooserEmbed *chooser_embed);
  /* Signals
   */
  void (*default_size_changed)    (BtkFileChooserEmbed *chooser_embed);
  void (*response_requested)      (BtkFileChooserEmbed *chooser_embed);
};

GType _btk_file_chooser_embed_get_type (void) B_GNUC_CONST;

void  _btk_file_chooser_embed_get_default_size    (BtkFileChooserEmbed *chooser_embed,
						   gint                *default_width,
						   gint                *default_height);
gboolean _btk_file_chooser_embed_should_respond (BtkFileChooserEmbed *chooser_embed);

void _btk_file_chooser_embed_initial_focus (BtkFileChooserEmbed *chooser_embed);

void _btk_file_chooser_embed_delegate_iface_init  (BtkFileChooserEmbedIface *iface);
void _btk_file_chooser_embed_set_delegate         (BtkFileChooserEmbed *receiver,
						   BtkFileChooserEmbed *delegate);

B_END_DECLS

#endif /* __BTK_FILE_CHOOSER_EMBED_H__ */
