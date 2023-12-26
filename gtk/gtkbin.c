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

/**
 * SECTION:btkbin
 * @Short_description: A container with just one child
 * @Title: BtkBin
 *
 * The #BtkBin widget is a container with just one child.
 * It is not very useful itself, but it is useful for deriving subclasses,
 * since it provides common code needed for handling a single child widget.
 *
 * Many BTK+ widgets are subclasses of #BtkBin, including #BtkWindow,
 * #BtkButton, #BtkFrame, #BtkHandleBox or #BtkScrolledWindow.
 */

#include "config.h"
#include "btkbin.h"
#include "btkintl.h"
#include "btkalias.h"

static void btk_bin_add         (BtkContainer   *container,
			         BtkWidget      *widget);
static void btk_bin_remove      (BtkContainer   *container,
			         BtkWidget      *widget);
static void btk_bin_forall      (BtkContainer   *container,
				 gboolean	include_internals,
				 BtkCallback     callback,
				 gpointer        callback_data);
static GType btk_bin_child_type (BtkContainer   *container);


G_DEFINE_ABSTRACT_TYPE (BtkBin, btk_bin, BTK_TYPE_CONTAINER)

static void
btk_bin_class_init (BtkBinClass *class)
{
  BtkContainerClass *container_class;

  container_class = (BtkContainerClass*) class;

  container_class->add = btk_bin_add;
  container_class->remove = btk_bin_remove;
  container_class->forall = btk_bin_forall;
  container_class->child_type = btk_bin_child_type;
}

static void
btk_bin_init (BtkBin *bin)
{
  btk_widget_set_has_window (BTK_WIDGET (bin), FALSE);

  bin->child = NULL;
}


static GType
btk_bin_child_type (BtkContainer *container)
{
  if (!BTK_BIN (container)->child)
    return BTK_TYPE_WIDGET;
  else
    return G_TYPE_NONE;
}

static void
btk_bin_add (BtkContainer *container,
	     BtkWidget    *child)
{
  BtkBin *bin = BTK_BIN (container);

  if (bin->child != NULL)
    {
      g_warning ("Attempting to add a widget with type %s to a %s, "
                 "but as a BtkBin subclass a %s can only contain one widget at a time; "
                 "it already contains a widget of type %s",
                 g_type_name (G_OBJECT_TYPE (child)),
                 g_type_name (G_OBJECT_TYPE (bin)),
                 g_type_name (G_OBJECT_TYPE (bin)),
                 g_type_name (G_OBJECT_TYPE (bin->child)));
      return;
    }

  btk_widget_set_parent (child, BTK_WIDGET (bin));
  bin->child = child;
}

static void
btk_bin_remove (BtkContainer *container,
		BtkWidget    *child)
{
  BtkBin *bin = BTK_BIN (container);
  gboolean widget_was_visible;

  g_return_if_fail (bin->child == child);

  widget_was_visible = btk_widget_get_visible (child);
  
  btk_widget_unparent (child);
  bin->child = NULL;
  
  /* queue resize regardless of btk_widget_get_visible (container),
   * since that's what is needed by toplevels, which derive from BtkBin.
   */
  if (widget_was_visible)
    btk_widget_queue_resize (BTK_WIDGET (container));
}

static void
btk_bin_forall (BtkContainer *container,
		gboolean      include_internals,
		BtkCallback   callback,
		gpointer      callback_data)
{
  BtkBin *bin = BTK_BIN (container);

  if (bin->child)
    (* callback) (bin->child, callback_data);
}

/**
 * btk_bin_get_child:
 * @bin: a #BtkBin
 * 
 * Gets the child of the #BtkBin, or %NULL if the bin contains
 * no child widget. The returned widget does not have a reference
 * added, so you do not need to unref it.
 *
 * Return value: (transfer none): pointer to child of the #BtkBin
 **/
BtkWidget*
btk_bin_get_child (BtkBin *bin)
{
  g_return_val_if_fail (BTK_IS_BIN (bin), NULL);

  return bin->child;
}

#define __BTK_BIN_C__
#include "btkaliasdef.c"
