/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 */
#ifndef __TICTACTOE_H__
#define __TICTACTOE_H__


#include <bunnylib.h>
#include <bunnylib-object.h>
#include <btk/btktable.h>


B_BEGIN_DECLS

#define TICTACTOE_TYPE            (tictactoe_get_type ())
#define TICTACTOE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TICTACTOE_TYPE, Tictactoe))
#define TICTACTOE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TICTACTOE_TYPE, TictactoeClass))
#define IS_TICTACTOE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TICTACTOE_TYPE))
#define IS_TICTACTOE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TICTACTOE_TYPE))


typedef struct _Tictactoe       Tictactoe;
typedef struct _TictactoeClass  TictactoeClass;

struct _Tictactoe
{
  BtkTable table;
  
  BtkWidget *buttons[3][3];
};

struct _TictactoeClass
{
  BtkTableClass parent_class;

  void (* tictactoe) (Tictactoe *ttt);
};

GType          tictactoe_get_type        (void);
BtkWidget*     tictactoe_new             (void);
void	       tictactoe_clear           (Tictactoe *ttt);

B_END_DECLS

#endif /* __TICTACTOE_H__ */

