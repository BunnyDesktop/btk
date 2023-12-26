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

#ifndef __BTK_ADJUSTMENT_H__
#define __BTK_ADJUSTMENT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkobject.h>

B_BEGIN_DECLS

#define BTK_TYPE_ADJUSTMENT                  (btk_adjustment_get_type ())
#define BTK_ADJUSTMENT(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_ADJUSTMENT, BtkAdjustment))
#define BTK_ADJUSTMENT_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_ADJUSTMENT, BtkAdjustmentClass))
#define BTK_IS_ADJUSTMENT(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_ADJUSTMENT))
#define BTK_IS_ADJUSTMENT_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_ADJUSTMENT))
#define BTK_ADJUSTMENT_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_ADJUSTMENT, BtkAdjustmentClass))


typedef struct _BtkAdjustment	    BtkAdjustment;
typedef struct _BtkAdjustmentClass  BtkAdjustmentClass;

struct _BtkAdjustment
{
  BtkObject parent_instance;

  bdouble GSEAL (lower);
  bdouble GSEAL (upper);
  bdouble GSEAL (value);
  bdouble GSEAL (step_increment);
  bdouble GSEAL (page_increment);
  bdouble GSEAL (page_size);
};

struct _BtkAdjustmentClass
{
  BtkObjectClass parent_class;

  void (* changed)	 (BtkAdjustment *adjustment);
  void (* value_changed) (BtkAdjustment *adjustment);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType	   btk_adjustment_get_type		(void) B_GNUC_CONST;
BtkObject* btk_adjustment_new			(bdouble	  value,
						 bdouble	  lower,
						 bdouble	  upper,
						 bdouble	  step_increment,
						 bdouble	  page_increment,
						 bdouble	  page_size);

void	   btk_adjustment_changed		(BtkAdjustment	 *adjustment);
void	   btk_adjustment_value_changed		(BtkAdjustment	 *adjustment);
void	   btk_adjustment_clamp_page		(BtkAdjustment	 *adjustment,
						 bdouble	  lower,
						 bdouble	  upper);

bdouble	   btk_adjustment_get_value		(BtkAdjustment   *adjustment);
void	   btk_adjustment_set_value		(BtkAdjustment	 *adjustment,
						 bdouble	  value);
bdouble    btk_adjustment_get_lower             (BtkAdjustment   *adjustment);
void       btk_adjustment_set_lower             (BtkAdjustment   *adjustment,
                                                 bdouble          lower);
bdouble    btk_adjustment_get_upper             (BtkAdjustment   *adjustment);
void       btk_adjustment_set_upper             (BtkAdjustment   *adjustment,
                                                 bdouble          upper);
bdouble    btk_adjustment_get_step_increment    (BtkAdjustment   *adjustment);
void       btk_adjustment_set_step_increment    (BtkAdjustment   *adjustment,
                                                 bdouble          step_increment);
bdouble    btk_adjustment_get_page_increment    (BtkAdjustment   *adjustment);
void       btk_adjustment_set_page_increment    (BtkAdjustment   *adjustment,
                                                 bdouble          page_increment);
bdouble    btk_adjustment_get_page_size         (BtkAdjustment   *adjustment);
void       btk_adjustment_set_page_size         (BtkAdjustment   *adjustment,
                                                 bdouble          page_size);

void       btk_adjustment_configure             (BtkAdjustment   *adjustment,
                                                 bdouble          value,
						 bdouble          lower,
						 bdouble          upper,
						 bdouble          step_increment,
						 bdouble          page_increment,
						 bdouble          page_size);

B_END_DECLS

#endif /* __BTK_ADJUSTMENT_H__ */
