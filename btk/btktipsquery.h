/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * BtkQueryTips: Query onscreen widgets for their tooltips
 * Copyright (C) 1998 Tim Janik
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

#ifndef BTK_DISABLE_DEPRECATED

#ifndef __BTK_TIPS_QUERY_H__
#define __BTK_TIPS_QUERY_H__


#include <btk/btk.h>


G_BEGIN_DECLS

/* --- type macros --- */
#define	BTK_TYPE_TIPS_QUERY		(btk_tips_query_get_type ())
#define BTK_TIPS_QUERY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TIPS_QUERY, BtkTipsQuery))
#define BTK_TIPS_QUERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TIPS_QUERY, BtkTipsQueryClass))
#define BTK_IS_TIPS_QUERY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TIPS_QUERY))
#define BTK_IS_TIPS_QUERY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TIPS_QUERY))
#define BTK_TIPS_QUERY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TIPS_QUERY, BtkTipsQueryClass))


/* --- typedefs --- */
typedef	struct	_BtkTipsQuery		BtkTipsQuery;
typedef	struct	_BtkTipsQueryClass	BtkTipsQueryClass;


/* --- structures --- */
struct	_BtkTipsQuery
{
  BtkLabel	label;

  guint		emit_always : 1;
  guint		in_query : 1;
  gchar		*label_inactive;
  gchar		*label_no_tip;

  BtkWidget	*caller;
  BtkWidget	*last_crossed;

  BdkCursor	*query_cursor;
};

struct	_BtkTipsQueryClass
{
  BtkLabelClass			parent_class;

  void	(*start_query)		(BtkTipsQuery	*tips_query);
  void	(*stop_query)		(BtkTipsQuery	*tips_query);
  void	(*widget_entered)	(BtkTipsQuery	*tips_query,
				 BtkWidget	*widget,
				 const gchar	*tip_text,
				 const gchar	*tip_private);
  gint	(*widget_selected)	(BtkTipsQuery	*tips_query,
				 BtkWidget	*widget,
				 const gchar	*tip_text,
				 const gchar	*tip_private,
				 BdkEventButton	*event);

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};


/* --- prototypes --- */
GType		btk_tips_query_get_type		(void) G_GNUC_CONST;
BtkWidget*	btk_tips_query_new		(void);
void		btk_tips_query_start_query	(BtkTipsQuery	*tips_query);
void		btk_tips_query_stop_query	(BtkTipsQuery	*tips_query);
void		btk_tips_query_set_caller	(BtkTipsQuery	*tips_query,
						 BtkWidget	*caller);
void		btk_tips_query_set_labels 	(BtkTipsQuery   *tips_query,
						 const gchar    *label_inactive,
						 const gchar    *label_no_tip);

G_END_DECLS

#endif	/* __BTK_TIPS_QUERY_H__ */

#endif /* BTK_DISABLE_DEPRECATED */
