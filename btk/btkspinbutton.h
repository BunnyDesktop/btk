/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkSpinButton widget for BTK+
 * Copyright (C) 1998 Lars Hamann and Stefan Jeske
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

#ifndef __BTK_SPIN_BUTTON_H__
#define __BTK_SPIN_BUTTON_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkentry.h>
#include <btk/btkadjustment.h>


B_BEGIN_DECLS

#define BTK_TYPE_SPIN_BUTTON                  (btk_spin_button_get_type ())
#define BTK_SPIN_BUTTON(obj)                  (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_SPIN_BUTTON, BtkSpinButton))
#define BTK_SPIN_BUTTON_CLASS(klass)          (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_SPIN_BUTTON, BtkSpinButtonClass))
#define BTK_IS_SPIN_BUTTON(obj)               (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_SPIN_BUTTON))
#define BTK_IS_SPIN_BUTTON_CLASS(klass)       (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_SPIN_BUTTON))
#define BTK_SPIN_BUTTON_GET_CLASS(obj)        (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_SPIN_BUTTON, BtkSpinButtonClass))

#define BTK_INPUT_ERROR -1

typedef enum
{
  BTK_UPDATE_ALWAYS,
  BTK_UPDATE_IF_VALID
} BtkSpinButtonUpdatePolicy;

typedef enum
{
  BTK_SPIN_STEP_FORWARD,
  BTK_SPIN_STEP_BACKWARD,
  BTK_SPIN_PAGE_FORWARD,
  BTK_SPIN_PAGE_BACKWARD,
  BTK_SPIN_HOME,
  BTK_SPIN_END,
  BTK_SPIN_USER_DEFINED
} BtkSpinType;


typedef struct _BtkSpinButton	    BtkSpinButton;
typedef struct _BtkSpinButtonClass  BtkSpinButtonClass;


struct _BtkSpinButton
{
  BtkEntry entry;

  BtkAdjustment *GSEAL (adjustment);

  BdkWindow *GSEAL (panel);

  buint32 GSEAL (timer);

  bdouble GSEAL (climb_rate);
  bdouble GSEAL (timer_step);

  BtkSpinButtonUpdatePolicy GSEAL (update_policy);

  buint GSEAL (in_child) : 2;
  buint GSEAL (click_child) : 2; /* valid: BTK_ARROW_UP=0, BTK_ARROW_DOWN=1 or 2=NONE/BOTH */
  buint GSEAL (button) : 2;
  buint GSEAL (need_timer) : 1;
  buint GSEAL (timer_calls) : 3;
  buint GSEAL (digits) : 10;
  buint GSEAL (numeric) : 1;
  buint GSEAL (wrap) : 1;
  buint GSEAL (snap_to_ticks) : 1;
};

struct _BtkSpinButtonClass
{
  BtkEntryClass parent_class;

  bint (*input)  (BtkSpinButton *spin_button,
		  bdouble       *new_value);
  bint (*output) (BtkSpinButton *spin_button);
  void (*value_changed) (BtkSpinButton *spin_button);

  /* Action signals for keybindings, do not connect to these */
  void (*change_value) (BtkSpinButton *spin_button,
			BtkScrollType  scroll);

  void (*wrapped) (BtkSpinButton *spin_button);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
};


GType		btk_spin_button_get_type	   (void) B_GNUC_CONST;

void		btk_spin_button_configure	   (BtkSpinButton  *spin_button,
						    BtkAdjustment  *adjustment,
						    bdouble	    climb_rate,
						    buint	    digits);

BtkWidget*	btk_spin_button_new		   (BtkAdjustment  *adjustment,
						    bdouble	    climb_rate,
						    buint	    digits);

BtkWidget*	btk_spin_button_new_with_range	   (bdouble  min,
						    bdouble  max,
						    bdouble  step);

void		btk_spin_button_set_adjustment	   (BtkSpinButton  *spin_button,
						    BtkAdjustment  *adjustment);

BtkAdjustment*	btk_spin_button_get_adjustment	   (BtkSpinButton  *spin_button);

void		btk_spin_button_set_digits	   (BtkSpinButton  *spin_button,
						    buint	    digits);
buint           btk_spin_button_get_digits         (BtkSpinButton  *spin_button);

void		btk_spin_button_set_increments	   (BtkSpinButton  *spin_button,
						    bdouble         step,
						    bdouble         page);
void            btk_spin_button_get_increments     (BtkSpinButton  *spin_button,
						    bdouble        *step,
						    bdouble        *page);

void		btk_spin_button_set_range	   (BtkSpinButton  *spin_button,
						    bdouble         min,
						    bdouble         max);
void            btk_spin_button_get_range          (BtkSpinButton  *spin_button,
						    bdouble        *min,
						    bdouble        *max);

bdouble		btk_spin_button_get_value          (BtkSpinButton  *spin_button);

bint		btk_spin_button_get_value_as_int   (BtkSpinButton  *spin_button);

void		btk_spin_button_set_value	   (BtkSpinButton  *spin_button,
						    bdouble	    value);

void		btk_spin_button_set_update_policy  (BtkSpinButton  *spin_button,
						    BtkSpinButtonUpdatePolicy  policy);
BtkSpinButtonUpdatePolicy btk_spin_button_get_update_policy (BtkSpinButton *spin_button);

void		btk_spin_button_set_numeric	   (BtkSpinButton  *spin_button,
						    bboolean	    numeric);
bboolean        btk_spin_button_get_numeric        (BtkSpinButton  *spin_button);

void		btk_spin_button_spin		   (BtkSpinButton  *spin_button,
						    BtkSpinType     direction,
						    bdouble	    increment);

void		btk_spin_button_set_wrap	   (BtkSpinButton  *spin_button,
						    bboolean	    wrap);
bboolean        btk_spin_button_get_wrap           (BtkSpinButton  *spin_button);

void		btk_spin_button_set_snap_to_ticks  (BtkSpinButton  *spin_button,
						    bboolean	    snap_to_ticks);
bboolean        btk_spin_button_get_snap_to_ticks  (BtkSpinButton  *spin_button);
void            btk_spin_button_update             (BtkSpinButton  *spin_button);


#ifndef BTK_DISABLE_DEPRECATED
#define btk_spin_button_get_value_as_float btk_spin_button_get_value
#endif

B_END_DECLS

#endif /* __BTK_SPIN_BUTTON_H__ */
