/* 
 * BTK - The GIMP Toolkit
 * Copyright (C) 1999  Red Hat, Inc.
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003  Matthias Clasen <mclasen@redhat.com>
 * Copyright (C) 2005  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
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

#ifndef __BTK_ASSISTANT_H__
#define __BTK_ASSISTANT_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <btk/btkwindow.h>

G_BEGIN_DECLS

#define BTK_TYPE_ASSISTANT         (btk_assistant_get_type ())
#define BTK_ASSISTANT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BTK_TYPE_ASSISTANT, BtkAssistant))
#define BTK_ASSISTANT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), BTK_TYPE_ASSISTANT, BtkAssistantClass))
#define BTK_IS_ASSISTANT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BTK_TYPE_ASSISTANT))
#define BTK_IS_ASSISTANT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), BTK_TYPE_ASSISTANT))
#define BTK_ASSISTANT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), BTK_TYPE_ASSISTANT, BtkAssistantClass))

/**
 * BtkAssistantPageType:
 * @BTK_ASSISTANT_PAGE_CONTENT: The page has regular contents.
 * @BTK_ASSISTANT_PAGE_INTRO: The page contains an introduction to the
 *  assistant task.
 * @BTK_ASSISTANT_PAGE_CONFIRM: The page lets the user confirm or deny the
 *  changes.
 * @BTK_ASSISTANT_PAGE_SUMMARY: The page informs the user of the changes
 *  done.
 * @BTK_ASSISTANT_PAGE_PROGRESS: Used for tasks that take a long time to
 *  complete, blocks the assistant until the page is marked as complete.
 *
 * An enum for determining the page role inside the #BtkAssistant. It's
 * used to handle buttons sensitivity and visibility.
 *
 * Note that an assistant needs to end its page flow with a page of type
 * %BTK_ASSISTANT_PAGE_CONFIRM, %BTK_ASSISTANT_PAGE_SUMMARY or
 * %BTK_ASSISTANT_PAGE_PROGRESS to be correct.
 */
typedef enum
{
  BTK_ASSISTANT_PAGE_CONTENT,
  BTK_ASSISTANT_PAGE_INTRO,
  BTK_ASSISTANT_PAGE_CONFIRM,
  BTK_ASSISTANT_PAGE_SUMMARY,
  BTK_ASSISTANT_PAGE_PROGRESS
} BtkAssistantPageType;

typedef struct _BtkAssistant        BtkAssistant;
typedef struct _BtkAssistantPrivate BtkAssistantPrivate;
typedef struct _BtkAssistantClass   BtkAssistantClass;

struct _BtkAssistant
{
  BtkWindow  parent;

  BtkWidget *GSEAL (cancel);
  BtkWidget *GSEAL (forward);
  BtkWidget *GSEAL (back);
  BtkWidget *GSEAL (apply);
  BtkWidget *GSEAL (close);
  BtkWidget *GSEAL (last);

  /*< private >*/
  BtkAssistantPrivate *GSEAL (priv);
};

struct _BtkAssistantClass
{
  BtkWindowClass parent_class;

  void (* prepare) (BtkAssistant *assistant, BtkWidget *page);
  void (* apply)   (BtkAssistant *assistant);
  void (* close)   (BtkAssistant *assistant);
  void (* cancel)  (BtkAssistant *assistant);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
  void (*_btk_reserved5) (void);
};

/**
 * BtkAssistantPageFunc:
 * @current_page: The page number used to calculate the next page.
 * @data: user data.
 *
 * A function used by btk_assistant_set_forward_page_func() to know which
 * is the next page given a current one. It's called both for computing the
 * next page when the user presses the "forward" button and for handling
 * the behavior of the "last" button.
 *
 * Returns: The next page number.
 */
typedef gint (*BtkAssistantPageFunc) (gint current_page, gpointer data);

GType                 btk_assistant_get_type              (void) G_GNUC_CONST;
BtkWidget            *btk_assistant_new                   (void);
gint                  btk_assistant_get_current_page      (BtkAssistant         *assistant);
void                  btk_assistant_set_current_page      (BtkAssistant         *assistant,
							   gint                  page_num);
gint                  btk_assistant_get_n_pages           (BtkAssistant         *assistant);
BtkWidget            *btk_assistant_get_nth_page          (BtkAssistant         *assistant,
							   gint                  page_num);
gint                  btk_assistant_prepend_page          (BtkAssistant         *assistant,
							   BtkWidget            *page);
gint                  btk_assistant_append_page           (BtkAssistant         *assistant,
							   BtkWidget            *page);
gint                  btk_assistant_insert_page           (BtkAssistant         *assistant,
							   BtkWidget            *page,
							   gint                  position);
void                  btk_assistant_set_forward_page_func (BtkAssistant         *assistant,
							   BtkAssistantPageFunc  page_func,
							   gpointer              data,
							   GDestroyNotify        destroy);
void                  btk_assistant_set_page_type         (BtkAssistant         *assistant,
							   BtkWidget            *page,
							   BtkAssistantPageType  type);
BtkAssistantPageType  btk_assistant_get_page_type         (BtkAssistant         *assistant,
							   BtkWidget            *page);
void                  btk_assistant_set_page_title        (BtkAssistant         *assistant,
							   BtkWidget            *page,
							   const gchar          *title);
const gchar *         btk_assistant_get_page_title        (BtkAssistant         *assistant,
							   BtkWidget            *page);
void                  btk_assistant_set_page_header_image (BtkAssistant         *assistant,
							   BtkWidget            *page,
							   BdkPixbuf            *pixbuf);
BdkPixbuf            *btk_assistant_get_page_header_image (BtkAssistant         *assistant,
							   BtkWidget            *page);
void                  btk_assistant_set_page_side_image   (BtkAssistant         *assistant,
							   BtkWidget            *page,
							   BdkPixbuf            *pixbuf);
BdkPixbuf            *btk_assistant_get_page_side_image   (BtkAssistant         *assistant,
							   BtkWidget            *page);
void                  btk_assistant_set_page_complete     (BtkAssistant         *assistant,
							   BtkWidget            *page,
							   gboolean              complete);
gboolean              btk_assistant_get_page_complete     (BtkAssistant         *assistant,
							   BtkWidget            *page);
void                  btk_assistant_add_action_widget     (BtkAssistant         *assistant,
							   BtkWidget            *child);
void                  btk_assistant_remove_action_widget  (BtkAssistant         *assistant,
							   BtkWidget            *child);

void                  btk_assistant_update_buttons_state  (BtkAssistant *assistant);
void                  btk_assistant_commit                (BtkAssistant *assistant);

G_END_DECLS

#endif /* __BTK_ASSISTANT_H__ */
