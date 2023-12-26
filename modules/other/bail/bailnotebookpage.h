/* BAIL - The BUNNY Accessibility Implementation Library
 * Copyright 2001, 2002, 2003 Sun Microsystems Inc.
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

#ifndef __BAIL_NOTEBOOK_PAGE_H__
#define __BAIL_NOTEBOOK_PAGE_H__

#include <bail/bailnotebook.h>
#include <libbail-util/bailtextutil.h>

B_BEGIN_DECLS

#define BAIL_TYPE_NOTEBOOK_PAGE            (bail_notebook_page_get_type ())
#define BAIL_NOTEBOOK_PAGE(obj)            (B_TYPE_CHECK_INSTANCE_CAST ((obj),BAIL_TYPE_NOTEBOOK_PAGE, BailNotebookPage))
#define BAIL_NOTEBOOK_PAGE_CLASS(klass)    (B_TYPE_CHECK_CLASS_CAST ((klass), BAIL_TYPE_NOTEBOOK_PAGE, BailNotebookPageClass))
#define BAIL_IS_NOTEBOOK_PAGE(obj)         (B_TYPE_CHECK_INSTANCE_TYPE ((obj), BAIL_TYPE_NOTEBOOK_PAGE))
#define BAIL_IS_NOTEBOOK_PAGE_CLASS(klass) (B_TYPE_CHECK_CLASS_TYPE ((klass), BAIL_TYPE_NOTEBOOK_PAGE))
#define BAIL_NOTEBOOK_PAGE_GET_CLASS(obj)  (B_TYPE_INSTANCE_GET_CLASS ((obj), BAIL_TYPE_NOTEBOOK_PAGE, BailNotebookPageClass))

typedef struct _BailNotebookPage                      BailNotebookPage;
typedef struct _BailNotebookPageClass                 BailNotebookPageClass;

struct _BailNotebookPage
{
  BatkObject parent;

  BtkNotebook *notebook;
#ifndef BTK_DISABLE_DEPRECATED
  BtkNotebookPage *page;
#else
  gpointer page;
#endif
  
  gint index;
  guint notify_child_added_id;

  BailTextUtil *textutil;
};

GType bail_notebook_page_get_type (void);

struct _BailNotebookPageClass
{
  BatkObjectClass parent_class;
};

BatkObject *bail_notebook_page_new(BtkNotebook *notebook, gint pagenum);

B_END_DECLS

#endif /* __BAIL_NOTEBOOK_PAGE_H__ */
