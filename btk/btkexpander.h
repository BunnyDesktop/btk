/* BTK - The GIMP Toolkit
 *
 * Copyright (C) 2003 Sun Microsystems, Inc.
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
 *
 * Authors:
 *	Mark McLoughlin <mark@skynet.ie>
 */

#ifndef __BTK_EXPANDER_H__
#define __BTK_EXPANDER_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkbin.h>

G_BEGIN_DECLS

#define BTK_TYPE_EXPANDER            (btk_expander_get_type ())
#define BTK_EXPANDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_EXPANDER, BtkExpander))
#define BTK_EXPANDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_EXPANDER, BtkExpanderClass))
#define BTK_IS_EXPANDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_EXPANDER))
#define BTK_IS_EXPANDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_EXPANDER))
#define BTK_EXPANDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_EXPANDER, BtkExpanderClass))

typedef struct _BtkExpander        BtkExpander;
typedef struct _BtkExpanderClass   BtkExpanderClass;
typedef struct _BtkExpanderPrivate BtkExpanderPrivate;

struct _BtkExpander
{
  BtkBin              bin;

  BtkExpanderPrivate *GSEAL (priv);
};

struct _BtkExpanderClass
{
  BtkBinClass    parent_class;

  /* Key binding signal; to get notification on the expansion
   * state connect to notify:expanded.
   */
  void        (* activate) (BtkExpander *expander);
};

GType                 btk_expander_get_type          (void) G_GNUC_CONST;

BtkWidget            *btk_expander_new               (const gchar *label);
BtkWidget            *btk_expander_new_with_mnemonic (const gchar *label);

void                  btk_expander_set_expanded      (BtkExpander *expander,
						      gboolean     expanded);
gboolean              btk_expander_get_expanded      (BtkExpander *expander);

/* Spacing between the expander/label and the child */
void                  btk_expander_set_spacing       (BtkExpander *expander,
						      gint         spacing);
gint                  btk_expander_get_spacing       (BtkExpander *expander);

void                  btk_expander_set_label         (BtkExpander *expander,
						      const gchar *label);
const gchar *         btk_expander_get_label         (BtkExpander *expander);

void                  btk_expander_set_use_underline (BtkExpander *expander,
						      gboolean     use_underline);
gboolean              btk_expander_get_use_underline (BtkExpander *expander);

void                  btk_expander_set_use_markup    (BtkExpander *expander,
						      gboolean    use_markup);
gboolean              btk_expander_get_use_markup    (BtkExpander *expander);

void                  btk_expander_set_label_widget  (BtkExpander *expander,
						      BtkWidget   *label_widget);
BtkWidget            *btk_expander_get_label_widget  (BtkExpander *expander);
void                  btk_expander_set_label_fill    (BtkExpander *expander,
						      gboolean     label_fill);
gboolean              btk_expander_get_label_fill    (BtkExpander *expander);

G_END_DECLS

#endif /* __BTK_EXPANDER_H__ */
