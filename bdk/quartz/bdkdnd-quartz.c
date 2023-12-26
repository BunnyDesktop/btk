/* bdkdnd-quartz.c
 *
 * Copyright (C) 2005 Imendio AB
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

#include "bdkdnd.h"
#include "bdkprivate-quartz.h"

static gpointer parent_class = NULL;

static void
bdk_drag_context_finalize (GObject *object)
{
  BdkDragContext *context = BDK_DRAG_CONTEXT (object);
  BdkDragContextPrivate *private = BDK_DRAG_CONTEXT_PRIVATE (context);
 
  g_free (private);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bdk_drag_context_init (BdkDragContext *dragcontext)
{
  BdkDragContextPrivate *priv = g_new0 (BdkDragContextPrivate, 1);

  dragcontext->windowing_data = priv;
}

static void
bdk_drag_context_class_init (BdkDragContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = bdk_drag_context_finalize;
}

GType
bdk_drag_context_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      const GTypeInfo object_info =
      {
        sizeof (BdkDragContextClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) bdk_drag_context_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (BdkDragContext),
        0,              /* n_preallocs */
        (GInstanceInitFunc) bdk_drag_context_init,
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "BdkDragContext",
                                            &object_info,
					    0);
    }
  
  return object_type;
}

BdkDragContext *
bdk_drag_context_new (void)
{
  return (BdkDragContext *)g_object_new (bdk_drag_context_get_type (), NULL);
}

void            
bdk_drag_context_ref (BdkDragContext *context)
{
  g_object_ref (context);
}

void            
bdk_drag_context_unref (BdkDragContext *context)
{
  g_object_unref (context);
}

BdkDragContext *_bdk_quartz_drag_source_context = NULL;

BdkDragContext *
bdk_quartz_drag_source_context ()
{
  return _bdk_quartz_drag_source_context;
}

BdkDragContext * 
bdk_drag_begin (BdkWindow     *window,
		GList         *targets)
{
  g_assert (_bdk_quartz_drag_source_context == NULL);
  
  /* Create fake context */
  _bdk_quartz_drag_source_context = bdk_drag_context_new ();
  _bdk_quartz_drag_source_context->is_source = TRUE;
  
  return _bdk_quartz_drag_source_context;
}

gboolean        
bdk_drag_motion (BdkDragContext *context,
		 BdkWindow      *dest_window,
		 BdkDragProtocol protocol,
		 gint            x_root, 
		 gint            y_root,
		 BdkDragAction   suggested_action,
		 BdkDragAction   possible_actions,
		 guint32         time)
{
  /* FIXME: Implement */
  return FALSE;
}

BdkNativeWindow
bdk_drag_get_protocol_for_display (BdkDisplay      *display,
				   BdkNativeWindow  xid,
				   BdkDragProtocol *protocol)
{
  /* FIXME: Implement */
  return 0;
}

void
bdk_drag_find_window_for_screen (BdkDragContext  *context,
				 BdkWindow       *drag_window,
				 BdkScreen       *screen,
				 gint             x_root,
				 gint             y_root,
				 BdkWindow      **dest_window,
				 BdkDragProtocol *protocol)
{
  /* FIXME: Implement */
}

void
bdk_drag_drop (BdkDragContext *context,
	       guint32         time)
{
  /* FIXME: Implement */
}

void
bdk_drag_abort (BdkDragContext *context,
		guint32         time)
{
  g_return_if_fail (context != NULL);
  
  /* FIXME: Implement */
}

void             
bdk_drag_status (BdkDragContext   *context,
		 BdkDragAction     action,
		 guint32           time)
{
  context->action = action;
}

void 
bdk_drop_reply (BdkDragContext   *context,
		gboolean          ok,
		guint32           time)
{
  g_return_if_fail (context != NULL);

  /* FIXME: Implement */
}

void             
bdk_drop_finish (BdkDragContext   *context,
		 gboolean          success,
		 guint32           time)
{
  /* FIXME: Implement */
}

void            
bdk_window_register_dnd (BdkWindow *window)
{
  /* FIXME: Implement */
}

BdkAtom       
bdk_drag_get_selection (BdkDragContext *context)
{
  /* FIXME: Implement */
  return BDK_NONE;
}

gboolean 
bdk_drag_drop_succeeded (BdkDragContext *context)
{
  /* FIXME: Implement */
  return FALSE;
}

id
bdk_quartz_drag_context_get_dragging_info_libbtk_only (BdkDragContext *context)
{
  return BDK_DRAG_CONTEXT_PRIVATE (context)->dragging_info;
}
