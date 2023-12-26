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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
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

#ifndef __BTK_RANGE_H__
#define __BTK_RANGE_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkadjustment.h>
#include <btk/btkwidget.h>


B_BEGIN_DECLS


#define BTK_TYPE_RANGE            (btk_range_get_type ())
#define BTK_RANGE(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_RANGE, BtkRange))
#define BTK_RANGE_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_RANGE, BtkRangeClass))
#define BTK_IS_RANGE(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_RANGE))
#define BTK_IS_RANGE_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_RANGE))
#define BTK_RANGE_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_RANGE, BtkRangeClass))

/* These two are private/opaque types, ignore */
typedef struct _BtkRangeLayout    BtkRangeLayout;
typedef struct _BtkRangeStepTimer BtkRangeStepTimer;

typedef struct _BtkRange        BtkRange;
typedef struct _BtkRangeClass   BtkRangeClass;

struct _BtkRange
{
  BtkWidget widget;

  BtkAdjustment *GSEAL (adjustment);
  BtkUpdateType GSEAL (update_policy);
  buint GSEAL (inverted) : 1;

  /*< protected >*/

  buint GSEAL (flippable) : 1;

  /* Steppers are: < > ---- < >
   *               a b      c d
   */

  buint GSEAL (has_stepper_a) : 1;
  buint GSEAL (has_stepper_b) : 1;
  buint GSEAL (has_stepper_c) : 1;
  buint GSEAL (has_stepper_d) : 1;

  buint GSEAL (need_recalc) : 1;

  buint GSEAL (slider_size_fixed) : 1;

  bint GSEAL (min_slider_size);

  BtkOrientation GSEAL (orientation);

  /* Area of entire stepper + trough assembly in widget->window coords */
  BdkRectangle GSEAL (range_rect);
  /* Slider range along the long dimension, in widget->window coords */
  bint GSEAL (slider_start);
  bint GSEAL (slider_end);

  /* Round off value to this many digits, -1 for no rounding */
  bint GSEAL (round_digits);

  /*< private >*/
  buint GSEAL (trough_click_forward) : 1;  /* trough click was on the forward side of slider */
  buint GSEAL (update_pending) : 1;        /* need to emit value_changed */
  BtkRangeLayout *GSEAL (layout);
  BtkRangeStepTimer *GSEAL (timer);
  bint GSEAL (slide_initial_slider_position);
  bint GSEAL (slide_initial_coordinate);
  buint GSEAL (update_timeout_id);
  BdkWindow *GSEAL (event_window);
};

struct _BtkRangeClass
{
  BtkWidgetClass parent_class;

  /* what detail to pass to BTK drawing functions */
  bchar *slider_detail;
  bchar *stepper_detail;

  void (* value_changed)    (BtkRange     *range);
  void (* adjust_bounds)    (BtkRange     *range,
                             bdouble	   new_value);

  /* action signals for keybindings */
  void (* move_slider)      (BtkRange     *range,
                             BtkScrollType scroll);

  /* Virtual functions */
  void (* get_range_border) (BtkRange     *range,
                             BtkBorder    *border_);

  bboolean (* change_value) (BtkRange     *range,
                             BtkScrollType scroll,
                             bdouble       new_value);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};


GType              btk_range_get_type                      (void) B_GNUC_CONST;

#ifndef BTK_DISABLE_DEPRECATED
void               btk_range_set_update_policy             (BtkRange      *range,
                                                            BtkUpdateType  policy);
BtkUpdateType      btk_range_get_update_policy             (BtkRange      *range);
#endif /* BTK_DISABLE_DEPRECATED */

void               btk_range_set_adjustment                (BtkRange      *range,
                                                            BtkAdjustment *adjustment);
BtkAdjustment*     btk_range_get_adjustment                (BtkRange      *range);

void               btk_range_set_inverted                  (BtkRange      *range,
                                                            bboolean       setting);
bboolean           btk_range_get_inverted                  (BtkRange      *range);

void               btk_range_set_flippable                 (BtkRange      *range,
                                                            bboolean       flippable);
bboolean           btk_range_get_flippable                 (BtkRange      *range);

void               btk_range_set_slider_size_fixed         (BtkRange      *range,
                                                            bboolean       size_fixed);
bboolean           btk_range_get_slider_size_fixed         (BtkRange      *range);

void               btk_range_set_min_slider_size           (BtkRange      *range,
                                                            bboolean       min_size);
bint               btk_range_get_min_slider_size           (BtkRange      *range);

void               btk_range_get_range_rect                (BtkRange      *range,
                                                            BdkRectangle  *range_rect);
void               btk_range_get_slider_range              (BtkRange      *range,
                                                            bint          *slider_start,
                                                            bint          *slider_end);

void               btk_range_set_lower_stepper_sensitivity (BtkRange      *range,
                                                            BtkSensitivityType sensitivity);
BtkSensitivityType btk_range_get_lower_stepper_sensitivity (BtkRange      *range);
void               btk_range_set_upper_stepper_sensitivity (BtkRange      *range,
                                                            BtkSensitivityType sensitivity);
BtkSensitivityType btk_range_get_upper_stepper_sensitivity (BtkRange      *range);

void               btk_range_set_increments                (BtkRange      *range,
                                                            bdouble        step,
                                                            bdouble        page);
void               btk_range_set_range                     (BtkRange      *range,
                                                            bdouble        min,
                                                            bdouble        max);
void               btk_range_set_value                     (BtkRange      *range,
                                                            bdouble        value);
bdouble            btk_range_get_value                     (BtkRange      *range);

void               btk_range_set_show_fill_level           (BtkRange      *range,
                                                            bboolean       show_fill_level);
bboolean           btk_range_get_show_fill_level           (BtkRange      *range);
void               btk_range_set_restrict_to_fill_level    (BtkRange      *range,
                                                            bboolean       restrict_to_fill_level);
bboolean           btk_range_get_restrict_to_fill_level    (BtkRange      *range);
void               btk_range_set_fill_level                (BtkRange      *range,
                                                            bdouble        fill_level);
bdouble            btk_range_get_fill_level                (BtkRange      *range);
void               btk_range_set_round_digits              (BtkRange      *range,
                                                            bint           round_digits);
bint                btk_range_get_round_digits             (BtkRange      *range);


/* internal API */
bdouble            _btk_range_get_wheel_delta              (BtkRange      *range,
                                                            BdkScrollDirection direction);

void               _btk_range_set_stop_values              (BtkRange      *range,
                                                            bdouble       *values,
                                                            bint           n_values);
bint               _btk_range_get_stop_positions           (BtkRange      *range,
                                                            bint         **values);          


B_END_DECLS


#endif /* __BTK_RANGE_H__ */
