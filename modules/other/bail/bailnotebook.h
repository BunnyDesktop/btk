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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __BAIL_NOTEBOOK_H__
#define __BAIL_NOTEBOOK_H__

#include <bail/bailcontainer.h>

B_BEGIN_DECLS

#define BAIL_TYPE_NOTEBOOK                   (bail_notebook_get_type ())
#define BAIL_NOTEBOOK(obj)                   (B_TYPE_CHECK_INSTANCE_CAST ((obj), BAIL_TYPE_NOTEBOOK, BailNotebook))
#define BAIL_NOTEBOOK_CLASS(klass)           (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_NOTEBOOK, BailNotebookClass))
#define BAIL_IS_NOTEBOOK(obj)                (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_NOTEBOOK))
#define BAIL_IS_NOTEBOOK_CLASS(klass)        (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_NOTEBOOK))
#define BAIL_NOTEBOOK_GET_CLASS(obj)         (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_NOTEBOOK, BailNotebookClass))

typedef struct _BailNotebook              BailNotebook;
typedef struct _BailNotebookClass         BailNotebookClass;

struct _BailNotebook
{
  BailContainer parent;

  /*
   * page_cache maintains a list of pre-ref'd Notebook Pages.
   * This cache is queried by bail_notebook_ref_child().
   * If the page is found in the list then a new page does not
   * need to be created
   */
  GList*       page_cache;
  gint         selected_page;
  gint         focus_tab_page;
  gint         page_count;
  guint        idle_focus_id;

  gint         remove_index;
};

GType bail_notebook_get_type (void);

struct _BailNotebookClass
{
  BailContainerClass parent_class;
};

B_END_DECLS

#endif /* __BAIL_NOTEBOOK_H__ */
