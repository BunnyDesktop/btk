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

#ifndef __BTK_PROGRESS_BAR_H__
#define __BTK_PROGRESS_BAR_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkprogress.h>


G_BEGIN_DECLS

#define BTK_TYPE_PROGRESS_BAR            (btk_progress_bar_get_type ())
#define BTK_PROGRESS_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_PROGRESS_BAR, BtkProgressBar))
#define BTK_PROGRESS_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_PROGRESS_BAR, BtkProgressBarClass))
#define BTK_IS_PROGRESS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_PROGRESS_BAR))
#define BTK_IS_PROGRESS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_PROGRESS_BAR))
#define BTK_PROGRESS_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_PROGRESS_BAR, BtkProgressBarClass))


typedef struct _BtkProgressBar       BtkProgressBar;
typedef struct _BtkProgressBarClass  BtkProgressBarClass;

typedef enum
{
  BTK_PROGRESS_CONTINUOUS,
  BTK_PROGRESS_DISCRETE
} BtkProgressBarStyle;

typedef enum
{
  BTK_PROGRESS_LEFT_TO_RIGHT,
  BTK_PROGRESS_RIGHT_TO_LEFT,
  BTK_PROGRESS_BOTTOM_TO_TOP,
  BTK_PROGRESS_TOP_TO_BOTTOM
} BtkProgressBarOrientation;

struct _BtkProgressBar
{
  BtkProgress progress;

  BtkProgressBarStyle       GSEAL (bar_style);
  BtkProgressBarOrientation GSEAL (orientation);

  guint GSEAL (blocks);
  gint  GSEAL (in_block);

  gint  GSEAL (activity_pos);
  guint GSEAL (activity_step);
  guint GSEAL (activity_blocks);

  gdouble GSEAL (pulse_fraction);

  guint GSEAL (activity_dir) : 1;
  guint GSEAL (ellipsize) : 3;
  guint GSEAL (dirty) : 1;
};

struct _BtkProgressBarClass
{
  BtkProgressClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


GType      btk_progress_bar_get_type             (void) G_GNUC_CONST;
BtkWidget* btk_progress_bar_new                  (void);

/*
 * BtkProgress/BtkProgressBar had serious problems in BTK 1.2.
 *
 *  - Only 3 or 4 functions are really needed for 95% of progress
 *    interfaces; BtkProgress[Bar] had about 25 functions, and
 *    didn't even include these 3 or 4.
 *  - In activity mode, the API involves setting the adjustment
 *    to any random value, just to have the side effect of
 *    calling the progress bar update function - the adjustment
 *    is totally ignored in activity mode
 *  - You set the activity step as a pixel value, which means to
 *    set the activity step you basically need to connect to
 *    size_allocate
 *  - There are ctree_set_expander_style()-functions, to randomly
 *    change look-and-feel for no good reason
 *  - The split between BtkProgress and BtkProgressBar makes no sense
 *    to me whatsoever.
 *
 * This was a big wart on BTK and made people waste lots of time,
 * both learning and using the interface.
 *
 * So, I have added what I feel is the correct API, and marked all the
 * rest deprecated. However, the changes are 100% backward-compatible and
 * should break no existing code.
 *
 * The following 9 functions are the new programming interface.
 */
void       btk_progress_bar_pulse                (BtkProgressBar *pbar);
void       btk_progress_bar_set_text             (BtkProgressBar *pbar,
                                                  const gchar    *text);
void       btk_progress_bar_set_fraction         (BtkProgressBar *pbar,
                                                  gdouble         fraction);

void       btk_progress_bar_set_pulse_step       (BtkProgressBar *pbar,
                                                  gdouble         fraction);
void       btk_progress_bar_set_orientation      (BtkProgressBar *pbar,
						  BtkProgressBarOrientation orientation);

const gchar*          btk_progress_bar_get_text       (BtkProgressBar *pbar);
gdouble               btk_progress_bar_get_fraction   (BtkProgressBar *pbar);
gdouble               btk_progress_bar_get_pulse_step (BtkProgressBar *pbar);

BtkProgressBarOrientation btk_progress_bar_get_orientation (BtkProgressBar *pbar);
void               btk_progress_bar_set_ellipsize (BtkProgressBar     *pbar,
						   BangoEllipsizeMode  mode);
BangoEllipsizeMode btk_progress_bar_get_ellipsize (BtkProgressBar     *pbar);


#ifndef BTK_DISABLE_DEPRECATED

/* Everything below here is deprecated */
BtkWidget* btk_progress_bar_new_with_adjustment  (BtkAdjustment  *adjustment);
void       btk_progress_bar_set_bar_style        (BtkProgressBar *pbar,
						  BtkProgressBarStyle style);
void       btk_progress_bar_set_discrete_blocks  (BtkProgressBar *pbar,
						  guint           blocks);
/* set_activity_step() is not only deprecated, it doesn't even work.
 * (Of course, it wasn't usable anyway, you had to set it from a size_allocate
 * handler or something)
 */
void       btk_progress_bar_set_activity_step    (BtkProgressBar *pbar,
                                                  guint           step);
void       btk_progress_bar_set_activity_blocks  (BtkProgressBar *pbar,
						  guint           blocks);
void       btk_progress_bar_update               (BtkProgressBar *pbar,
						  gdouble         percentage);

#endif /* BTK_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __BTK_PROGRESS_BAR_H__ */
