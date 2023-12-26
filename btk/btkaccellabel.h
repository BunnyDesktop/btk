/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkAccelLabel: BtkLabel with accelerator monitoring facilities.
 * Copyright (C) 1998 Tim Janik
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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/.
 */

#ifndef __BTK_ACCEL_LABEL_H__
#define __BTK_ACCEL_LABEL_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btklabel.h>


B_BEGIN_DECLS

#define BTK_TYPE_ACCEL_LABEL		(btk_accel_label_get_type ())
#define BTK_ACCEL_LABEL(obj)		(B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ACCEL_LABEL, BtkAccelLabel))
#define BTK_ACCEL_LABEL_CLASS(klass)	(B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ACCEL_LABEL, BtkAccelLabelClass))
#define BTK_IS_ACCEL_LABEL(obj)		(B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ACCEL_LABEL))
#define BTK_IS_ACCEL_LABEL_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ACCEL_LABEL))
#define BTK_ACCEL_LABEL_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ACCEL_LABEL, BtkAccelLabelClass))


typedef struct _BtkAccelLabel	    BtkAccelLabel;
typedef struct _BtkAccelLabelClass  BtkAccelLabelClass;

/**
 * BtkAccelLabel:
 *
 * The #BtkAccelLabel-struct struct contains private data only, and
 * should be accessed using the functions below.
 */
struct _BtkAccelLabel
{
  BtkLabel label;

  buint          GSEAL (btk_reserved);
  buint          GSEAL (accel_padding);      /* should be style property? */
  BtkWidget     *GSEAL (accel_widget);       /* done*/
  GClosure      *GSEAL (accel_closure);      /* has set function */
  BtkAccelGroup *GSEAL (accel_group);        /* set by set_accel_closure() */
  bchar         *GSEAL (accel_string);       /* has set function */
  buint16        GSEAL (accel_string_width); /* seems to be private */
};

struct _BtkAccelLabelClass
{
  BtkLabelClass	 parent_class;

  bchar		*signal_quote1;
  bchar		*signal_quote2;
  bchar		*mod_name_shift;
  bchar		*mod_name_control;
  bchar		*mod_name_alt;
  bchar		*mod_separator;
  bchar		*accel_seperator;
  buint		 latin1_to_char : 1;
  
  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

#ifndef BTK_DISABLE_DEPRECATED
#define	btk_accel_label_accelerator_width	btk_accel_label_get_accel_width
#endif /* BTK_DISABLE_DEPRECATED */

GType	   btk_accel_label_get_type	     (void) B_GNUC_CONST;
BtkWidget* btk_accel_label_new		     (const bchar   *string);
BtkWidget* btk_accel_label_get_accel_widget  (BtkAccelLabel *accel_label);
buint	   btk_accel_label_get_accel_width   (BtkAccelLabel *accel_label);
void	   btk_accel_label_set_accel_widget  (BtkAccelLabel *accel_label,
					      BtkWidget	    *accel_widget);
void	   btk_accel_label_set_accel_closure (BtkAccelLabel *accel_label,
					      GClosure	    *accel_closure);
bboolean   btk_accel_label_refetch           (BtkAccelLabel *accel_label);

/* private */
bchar *    _btk_accel_label_class_get_accelerator_label (BtkAccelLabelClass *klass,
							 buint               accelerator_key,
							 BdkModifierType     accelerator_mods);

B_END_DECLS

#endif /* __BTK_ACCEL_LABEL_H__ */
