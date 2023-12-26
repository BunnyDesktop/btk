/*
 * btkimmodulequartz
 * Copyright (C) 2011 Hiroyuki Yamamoto
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
 *
 * $Id:$
 */

#include "config.h"
#include <string.h>

#include <btk/btk.h>
#include "btk/btkintl.h"
#include "btk/btkimmodule.h"

#include "bdk/quartz/bdkquartz.h"
#include "bdk/quartz/BdkQuartzView.h"

#define BTK_IM_CONTEXT_TYPE_QUARTZ (type_quartz)
#define BTK_IM_CONTEXT_QUARTZ(obj) (B_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_IM_CONTEXT_TYPE_QUARTZ, BtkIMContextQuartz))
#define BTK_IM_CONTEXT_QUARTZ_GET_CLASS(obj) (B_TYPE_INSTANCE_GET_CLASS((obj), BTK_IM_CONTEXT_TYPE_QUARTZ, BtkIMContextQuartzClass))

typedef struct _BtkIMContextQuartz
{
  BtkIMContext parent;
  BtkIMContext *slave;
  BdkWindow *client_window;
  gchar *preedit_str;
  unsigned int cursor_index;
  unsigned int selected_len;
  BdkRectangle *cursor_rect;
  gboolean focused;
} BtkIMContextQuartz;

typedef struct _BtkIMContextQuartzClass
{
  BtkIMContextClass parent_class;
} BtkIMContextQuartzClass;

GType type_quartz = 0;
static BObjectClass *parent_class;

static const BtkIMContextInfo imquartz_info =
{
  "quartz",
  "Mac OS X Quartz",
  GETTEXT_PACKAGE,
  BTK_LOCALEDIR,
  "ja:ko:zh:*",
};

static const BtkIMContextInfo *info_list[] =
{
  &imquartz_info,
};

#ifndef INCLUDE_IM_quartz
#define MODULE_ENTRY(type,function) G_MODULE_EXPORT type im_module_ ## function
#else
#define MODULE_ENTRY(type, function) type _btk_immodule_quartz_ ## function
#endif

static void
quartz_get_preedit_string (BtkIMContext *context,
                           gchar **str,
                           BangoAttrList **attrs,
                           gint *cursor_pos)
{
  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);

  BTK_NOTE (MISC, g_print ("quartz_get_preedit_string\n"));

  if (str)
    *str = qc->preedit_str ? g_strdup (qc->preedit_str) : g_strdup ("");

  if (attrs)
    {
      *attrs = bango_attr_list_new ();
      int len = g_utf8_strlen (*str, -1);
      gchar *ch = *str;
      if (len > 0)
        {
          BangoAttribute *attr;
          int i = 0;
          for (;;)
            {
              gchar *s = ch;
              ch = g_utf8_next_char (ch);

              if (i >= qc->cursor_index &&
		  i < qc->cursor_index + qc->selected_len)
                attr = bango_attr_underline_new (BANGO_UNDERLINE_DOUBLE);
              else
                attr = bango_attr_underline_new (BANGO_UNDERLINE_SINGLE);

              attr->start_index = s - *str;
              if (!*ch)
                attr->end_index = attr->start_index + strlen (s);
              else
                attr->end_index = ch - *str;

              bango_attr_list_change (*attrs, attr);

              if (!*ch)
                break;
              i++;
            }
        }
    }
  if (cursor_pos)
    *cursor_pos = qc->cursor_index;
}

static gboolean
output_result (BtkIMContext *context,
               BdkWindow *win)
{
  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);
  gboolean retval = FALSE;
  gchar *fixed_str, *marked_str;

  fixed_str = g_object_get_data (B_OBJECT (win), TIC_INSERT_TEXT);
  marked_str = g_object_get_data (B_OBJECT (win), TIC_MARKED_TEXT);
  if (fixed_str)
    {
      BTK_NOTE (MISC, g_print ("tic-insert-text: %s\n", fixed_str));
      g_free (qc->preedit_str);
      qc->preedit_str = NULL;
      g_object_set_data (B_OBJECT (win), TIC_INSERT_TEXT, NULL);
      g_signal_emit_by_name (context, "commit", fixed_str);
      g_signal_emit_by_name (context, "preedit_changed");

      unsigned int filtered =
	   GPOINTER_TO_UINT (g_object_get_data (B_OBJECT (win),
						GIC_FILTER_KEY));
      BTK_NOTE (MISC, g_print ("filtered, %d\n", filtered));
      if (filtered)
        retval = TRUE;
      else
        retval = FALSE;
    }
  if (marked_str)
    {
      BTK_NOTE (MISC, g_print ("tic-marked-text: %s\n", marked_str));
      qc->cursor_index =
	   GPOINTER_TO_UINT (g_object_get_data (B_OBJECT (win),
						TIC_SELECTED_POS));
      qc->selected_len =
	   GPOINTER_TO_UINT (g_object_get_data (B_OBJECT (win),
						TIC_SELECTED_LEN));
      g_free (qc->preedit_str);
      qc->preedit_str = g_strdup (marked_str);
      g_object_set_data (B_OBJECT (win), TIC_MARKED_TEXT, NULL);
      g_signal_emit_by_name (context, "preedit_changed");
      retval = TRUE;
    }
  if (!fixed_str && !marked_str)
    {
      if (qc->preedit_str && strlen (qc->preedit_str) > 0)
        retval = TRUE;
    }

  g_free (fixed_str);
  g_free (marked_str);

  return retval;
}

static gboolean
quartz_filter_keypress (BtkIMContext *context,
                        BdkEventKey *event)
{
  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);
  gboolean retval;
  NSView *nsview;
  BdkWindow *win;

  BTK_NOTE (MISC, g_print ("quartz_filter_keypress\n"));

  if (!qc->client_window)
    return FALSE;

  if (!bdk_quartz_window_is_quartz (qc->client_window))
    return btk_im_context_filter_keypress (qc->slave, event);

  nsview = bdk_quartz_window_get_nsview (qc->client_window);
  win = (BdkWindow *)[ (BdkQuartzView *)nsview bdkWindow];
  BTK_NOTE (MISC, g_print ("client_window: %p, win: %p, nsview: %p\n",
			   qc->client_window, win, nsview));

  NSEvent *nsevent = bdk_quartz_event_get_nsevent ((BdkEvent *)event);

  if (!nsevent)
    {
      if (event->hardware_keycode == 0 && event->keyval == 0xffffff)
        /* update text input changes by mouse events */
        return output_result (context, win);
      else
        return btk_im_context_filter_keypress (qc->slave, event);
    }

  if (event->type == BDK_KEY_RELEASE)
    return FALSE;

  if (event->hardware_keycode == 55)	/* Command */
    return FALSE;

  NSEventType etype = [nsevent type];
  if (etype == NSKeyDown)
    {
       g_object_set_data (B_OBJECT (win), TIC_IN_KEY_DOWN,
                                          GUINT_TO_POINTER (TRUE));
       [nsview keyDown: nsevent];
    }
  /* JIS_Eisu || JIS_Kana */
  if (event->hardware_keycode == 102 || event->hardware_keycode == 104)
    return FALSE;

  retval = output_result(context, win);
  g_object_set_data (B_OBJECT (win), TIC_IN_KEY_DOWN,
                                     GUINT_TO_POINTER (FALSE));
  BTK_NOTE (MISC, g_print ("quartz_filter_keypress done\n"));

  return retval;
}

static void
discard_preedit (BtkIMContext *context)
{
  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);

  if (!qc->client_window)
    return;

  if (!bdk_quartz_window_is_quartz (qc->client_window))
    return;

  NSView *nsview = bdk_quartz_window_get_nsview (qc->client_window);
  if (!nsview)
    return;

  /* reset any partial input for this NSView */
  [(BdkQuartzView *)nsview unmarkText];
  NSInputManager *currentInputManager = [NSInputManager currentInputManager];
  [currentInputManager markedTextAbandoned:nsview];

  if (qc->preedit_str && strlen (qc->preedit_str) > 0)
    {
      g_signal_emit_by_name (context, "commit", qc->preedit_str);

      g_free (qc->preedit_str);
      qc->preedit_str = NULL;
      g_signal_emit_by_name (context, "preedit_changed");
    }
}

static void
quartz_reset (BtkIMContext *context)
{
  BTK_NOTE (MISC, g_print ("quartz_reset\n"));
  discard_preedit (context);
}

static void
quartz_set_client_window (BtkIMContext *context, BdkWindow *window)
{
  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);

  BTK_NOTE (MISC, g_print ("quartz_set_client_window: %p\n", window));

  qc->client_window = window;
}

static void
quartz_focus_in (BtkIMContext *context)
{
  BTK_NOTE (MISC, g_print ("quartz_focus_in\n"));

  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);
  qc->focused = TRUE;
}

static void
quartz_focus_out (BtkIMContext *context)
{
  BTK_NOTE (MISC, g_print ("quartz_focus_out\n"));

  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);
  qc->focused = FALSE;

  /* Commit any partially built strings or it'll mess up other BTK+ widgets in the window */
  discard_preedit (context);
}

static void
quartz_set_cursor_location (BtkIMContext *context, BdkRectangle *area)
{
  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (context);
  gint x, y;
  NSView *nsview;
  BdkWindow *win;

  BTK_NOTE (MISC, g_print ("quartz_set_cursor_location\n"));

  if (!qc->client_window)
    return;

  if (!bdk_quartz_window_is_quartz (qc->client_window))
    return;

  if (!qc->focused)
    return;

  qc->cursor_rect->x = area->x;
  qc->cursor_rect->y = area->y;
  qc->cursor_rect->width = area->width;
  qc->cursor_rect->height = area->height;

  bdk_window_get_origin (qc->client_window, &x, &y);

  qc->cursor_rect->x = area->x + x;
  qc->cursor_rect->y = area->y + y;

  nsview = bdk_quartz_window_get_nsview (qc->client_window);

  win = (BdkWindow *)[ (BdkQuartzView*)nsview bdkWindow];
  g_object_set_data (B_OBJECT (win), GIC_CURSOR_RECT, qc->cursor_rect);
}

static void
quartz_set_use_preedit (BtkIMContext *context, gboolean use_preedit)
{
  BTK_NOTE (MISC, g_print ("quartz_set_use_preedit: %d\n", use_preedit));
}

static void
commit_cb (BtkIMContext *context, const gchar *str, BtkIMContextQuartz *qc)
{
  g_signal_emit_by_name (qc, "commit", str);
}

static void
imquartz_finalize (BObject *obj)
{
  BTK_NOTE (MISC, g_print ("imquartz_finalize\n"));

  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (obj);
  g_free (qc->preedit_str);
  qc->preedit_str = NULL;
  g_free (qc->cursor_rect);
  qc->cursor_rect = NULL;

  g_signal_handlers_disconnect_by_func (qc->slave, (gpointer)commit_cb, qc);
  g_object_unref (qc->slave);

  parent_class->finalize (obj);
}

static void
btk_im_context_quartz_class_init (BtkIMContextClass *klass)
{
  BTK_NOTE (MISC, g_print ("btk_im_context_quartz_class_init\n"));

  BObjectClass *object_class = B_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  klass->get_preedit_string = quartz_get_preedit_string;
  klass->filter_keypress = quartz_filter_keypress;
  klass->reset = quartz_reset;
  klass->set_client_window = quartz_set_client_window;
  klass->focus_in = quartz_focus_in;
  klass->focus_out = quartz_focus_out;
  klass->set_cursor_location = quartz_set_cursor_location;
  klass->set_use_preedit = quartz_set_use_preedit;

  object_class->finalize = imquartz_finalize;
}

static void
btk_im_context_quartz_init (BtkIMContext *im_context)
{
  BTK_NOTE (MISC, g_print ("btk_im_context_quartz_init\n"));

  BtkIMContextQuartz *qc = BTK_IM_CONTEXT_QUARTZ (im_context);
  qc->preedit_str = g_strdup ("");
  qc->cursor_index = 0;
  qc->selected_len = 0;
  qc->cursor_rect = g_malloc (sizeof (BdkRectangle));
  qc->focused = FALSE;

  qc->slave = g_object_new (BTK_TYPE_IM_CONTEXT_SIMPLE, NULL);
  g_signal_connect (B_OBJECT (qc->slave), "commit", G_CALLBACK (commit_cb), qc);
}

static void
btk_im_context_quartz_register_type (GTypeModule *module)
{
  const GTypeInfo object_info =
  {
    sizeof (BtkIMContextQuartzClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) btk_im_context_quartz_class_init,
    NULL,           /* class_finalize */
    NULL,           /* class_data */
    sizeof (BtkIMContextQuartz),
    0,
    (GInstanceInitFunc) btk_im_context_quartz_init,
  };

  type_quartz =
    g_type_module_register_type (module,
                                 BTK_TYPE_IM_CONTEXT,
                                 "BtkIMContextQuartz",
                                 &object_info, 0);
}

MODULE_ENTRY (void, init) (GTypeModule * module)
{
  btk_im_context_quartz_register_type (module);
}

MODULE_ENTRY (void, exit) (void)
{
}

MODULE_ENTRY (void, list) (const BtkIMContextInfo *** contexts, int *n_contexts)
{
  *contexts = info_list;
  *n_contexts = G_N_ELEMENTS (info_list);
}

MODULE_ENTRY (BtkIMContext *, create) (const gchar * context_id)
{
  g_return_val_if_fail (context_id, NULL);

  if (!strcmp (context_id, "quartz"))
    {
      BTK_NOTE (MISC, g_print ("immodule_quartz create\n"));
      return g_object_new (type_quartz, NULL);
    }
  else
    return NULL;
}
