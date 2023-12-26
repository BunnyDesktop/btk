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

#ifndef __BTK_TEXT_TAG_TABLE_H__
#define __BTK_TEXT_TAG_TABLE_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btktexttag.h>

B_BEGIN_DECLS

typedef void (* BtkTextTagTableForeach) (BtkTextTag *tag, gpointer data);

#define BTK_TYPE_TEXT_TAG_TABLE            (btk_text_tag_table_get_type ())
#define BTK_TEXT_TAG_TABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT_TAG_TABLE, BtkTextTagTable))
#define BTK_TEXT_TAG_TABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_TAG_TABLE, BtkTextTagTableClass))
#define BTK_IS_TEXT_TAG_TABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT_TAG_TABLE))
#define BTK_IS_TEXT_TAG_TABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_TAG_TABLE))
#define BTK_TEXT_TAG_TABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_TAG_TABLE, BtkTextTagTableClass))

typedef struct _BtkTextTagTableClass BtkTextTagTableClass;

struct _BtkTextTagTable
{
  GObject parent_instance;

  GHashTable *GSEAL (hash);
  GSList *GSEAL (anonymous);
  gint GSEAL (anon_count);

  GSList *GSEAL (buffers);
};

struct _BtkTextTagTableClass
{
  GObjectClass parent_class;

  void (* tag_changed) (BtkTextTagTable *table, BtkTextTag *tag, gboolean size_changed);
  void (* tag_added) (BtkTextTagTable *table, BtkTextTag *tag);
  void (* tag_removed) (BtkTextTagTable *table, BtkTextTag *tag);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType          btk_text_tag_table_get_type (void) B_GNUC_CONST;

BtkTextTagTable *btk_text_tag_table_new      (void);
void             btk_text_tag_table_add      (BtkTextTagTable        *table,
                                              BtkTextTag             *tag);
void             btk_text_tag_table_remove   (BtkTextTagTable        *table,
                                              BtkTextTag             *tag);
BtkTextTag      *btk_text_tag_table_lookup   (BtkTextTagTable        *table,
                                              const gchar            *name);
void             btk_text_tag_table_foreach  (BtkTextTagTable        *table,
                                              BtkTextTagTableForeach  func,
                                              gpointer                data);
gint             btk_text_tag_table_get_size (BtkTextTagTable        *table);


/* INTERNAL private stuff - not even exported from the library on
 * many platforms
 */
void _btk_text_tag_table_add_buffer    (BtkTextTagTable *table,
                                        gpointer         buffer);
void _btk_text_tag_table_remove_buffer (BtkTextTagTable *table,
                                        gpointer         buffer);

B_END_DECLS

#endif

