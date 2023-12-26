/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat Software
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

#ifndef __BTK_IM_CONTEXT_H__
#define __BTK_IM_CONTEXT_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkobject.h>


G_BEGIN_DECLS

#define BTK_TYPE_IM_CONTEXT              (btk_im_context_get_type ())
#define BTK_IM_CONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_IM_CONTEXT, BtkIMContext))
#define BTK_IM_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_IM_CONTEXT, BtkIMContextClass))
#define BTK_IS_IM_CONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_IM_CONTEXT))
#define BTK_IS_IM_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_IM_CONTEXT))
#define BTK_IM_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_IM_CONTEXT, BtkIMContextClass))


typedef struct _BtkIMContext       BtkIMContext;
typedef struct _BtkIMContextClass  BtkIMContextClass;

struct _BtkIMContext
{
  GObject parent_instance;
};

struct _BtkIMContextClass
{
  /*< private >*/
  /* Yes, this should be GObjectClass, be we can't fix it without breaking
   * binary compatibility - see bug #90935
   */
  BtkObjectClass parent_class;

  /*< public >*/
  /* Signals */
  void     (*preedit_start)        (BtkIMContext *context);
  void     (*preedit_end)          (BtkIMContext *context);
  void     (*preedit_changed)      (BtkIMContext *context);
  void     (*commit)               (BtkIMContext *context, const gchar *str);
  gboolean (*retrieve_surrounding) (BtkIMContext *context);
  gboolean (*delete_surrounding)   (BtkIMContext *context,
				    gint          offset,
				    gint          n_chars);

  /* Virtual functions */
  void     (*set_client_window)   (BtkIMContext   *context,
				   BdkWindow      *window);
  void     (*get_preedit_string)  (BtkIMContext   *context,
				   gchar         **str,
				   BangoAttrList **attrs,
				   gint           *cursor_pos);
  gboolean (*filter_keypress)     (BtkIMContext   *context,
			           BdkEventKey    *event);
  void     (*focus_in)            (BtkIMContext   *context);
  void     (*focus_out)           (BtkIMContext   *context);
  void     (*reset)               (BtkIMContext   *context);
  void     (*set_cursor_location) (BtkIMContext   *context,
				   BdkRectangle   *area);
  void     (*set_use_preedit)     (BtkIMContext   *context,
				   gboolean        use_preedit);
  void     (*set_surrounding)     (BtkIMContext   *context,
				   const gchar    *text,
				   gint            len,
				   gint            cursor_index);
  gboolean (*get_surrounding)     (BtkIMContext   *context,
				   gchar         **text,
				   gint           *cursor_index);
  /*< private >*/
  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
  void (*_btk_reserved6) (void);
};

GType    btk_im_context_get_type            (void) G_GNUC_CONST;

void     btk_im_context_set_client_window   (BtkIMContext       *context,
					     BdkWindow          *window);
void     btk_im_context_get_preedit_string  (BtkIMContext       *context,
					     gchar             **str,
					     BangoAttrList     **attrs,
					     gint               *cursor_pos);
gboolean btk_im_context_filter_keypress     (BtkIMContext       *context,
					     BdkEventKey        *event);
void     btk_im_context_focus_in            (BtkIMContext       *context);
void     btk_im_context_focus_out           (BtkIMContext       *context);
void     btk_im_context_reset               (BtkIMContext       *context);
void     btk_im_context_set_cursor_location (BtkIMContext       *context,
					     const BdkRectangle *area);
void     btk_im_context_set_use_preedit     (BtkIMContext       *context,
					     gboolean            use_preedit);
void     btk_im_context_set_surrounding     (BtkIMContext       *context,
					     const gchar        *text,
					     gint                len,
					     gint                cursor_index);
gboolean btk_im_context_get_surrounding     (BtkIMContext       *context,
					     gchar             **text,
					     gint               *cursor_index);
gboolean btk_im_context_delete_surrounding  (BtkIMContext       *context,
					     gint                offset,
					     gint                n_chars);

G_END_DECLS

#endif /* __BTK_IM_CONTEXT_H__ */
