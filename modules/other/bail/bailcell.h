/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001 Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_CELL_H__
#define __BAIL_CELL_H__

#include <batk/batk.h>

G_BEGIN_DECLS

#define BAIL_TYPE_CELL                           (bail_cell_get_type ())
#define BAIL_CELL(obj)                           (G_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_CELL, BailCell))
#define BAIL_CELL_CLASS(klass)                   (G_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_CELL, BailCellClass))
#define BAIL_IS_CELL(obj)                        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_CELL))
#define BAIL_IS_CELL_CLASS(klass)                (G_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_CELL))
#define BAIL_CELL_GET_CLASS(obj)                 (G_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_CELL, BailCellClass))

typedef struct _BailCell                  BailCell;
typedef struct _BailCellClass             BailCellClass;
typedef struct _ActionInfo ActionInfo;
typedef void (*ACTION_FUNC) (BailCell *cell);
  
struct _BailCell
{
  BatkObject parent;

  BtkWidget    *widget;
  /*
   * This cached value is used only by batk_object_get_index_in_parent()
   * which updates the value when it is stale.
   */
  gint         index;
  BatkStateSet *state_set;
  GList       *action_list;
  void (*refresh_index) (BailCell *cell);
  gint         action_idle_handler;
  ACTION_FUNC  action_func;
};

GType bail_cell_get_type (void);

struct _BailCellClass
{
  BatkObjectClass parent_class;
};

struct _ActionInfo {
  gchar *name;
  gchar *description;
  gchar *keybinding;
  ACTION_FUNC do_action_func;
};

void      bail_cell_initialise           (BailCell        *cell,
                                          BtkWidget       *widget, 
                                          BatkObject       *parent,
			                  gint            index);

gboolean bail_cell_add_state             (BailCell        *cell,
                                          BatkStateType    state_type,
                                          gboolean        emit_signal);

gboolean bail_cell_remove_state          (BailCell        *cell,
                                          BatkStateType    state_type,
                                          gboolean        emit_signal);

#ifndef BTK_DISABLE_DEPRECATED
void     bail_cell_type_add_action_interface 
                                         (GType           type);
#endif

gboolean bail_cell_add_action            (BailCell        *cell,
		                          const gchar     *action_name,
		                          const gchar     *action_description,
		                          const gchar     *action_keybinding,
		                          ACTION_FUNC     action_func);

gboolean bail_cell_remove_action         (BailCell        *cell,
                                          gint            action_id);

gboolean bail_cell_remove_action_by_name (BailCell        *cell,
                                          const gchar     *action_name);

G_END_DECLS

#endif /* __BAIL_CELL_H__ */
