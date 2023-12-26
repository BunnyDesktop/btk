/* BTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
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

#undef BTK_DISABLE_DEPRECATED

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#undef BDK_PIXBUF_DISABLE_DEPRECATED
#include <bdk-pixbuf/bdk-pixdata.h>

#include "btkgamma.h"
#include "btkcurve.h"
#include "btkdialog.h"
#include "btkdrawingarea.h"
#include "btkentry.h"
#include "btkhbox.h"
#include "btkimage.h"
#include "btklabel.h"
#include "btkmain.h"
#include "btkradiobutton.h"
#include "btkstock.h"
#include "btktable.h"
#include "btkvbox.h"
#include "btkwindow.h"
#include "btkintl.h"
#include "btkalias.h"

/* forward declarations: */
static void btk_gamma_curve_destroy (BtkObject *object);

static void curve_type_changed_callback (BtkWidget *w, gpointer data);
static void button_realize_callback     (BtkWidget *w);
static void button_toggled_callback     (BtkWidget *w, gpointer data);
static void button_clicked_callback     (BtkWidget *w, gpointer data);

enum
  {
    LINEAR = 0,
    SPLINE,
    FREE,
    GAMMA,
    RESET,
    NUM_XPMS
  };

/* BdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (spline_pixdata)
#endif
#ifdef __GNUC__
static const guint8 spline_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 spline_pixdata[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "BdkP"
  /* length: header (24) + pixel_data (182) */
  "\0\0\0\316"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\216\0\0\0\0\202\0\0\0\377\211\0\0\0\0\206\377\0\0\377\1\0\0\0\377\207"
  "\0\0\0\0\202\377\0\0\377\214\0\0\0\0\2\0\0\0\377\274--\377\215\0\0\0"
  "\0\203\0\0\0\377\215\0\0\0\0\2\274--\377\0\0\0\377\216\0\0\0\0\1\377"
  "\0\0\377\216\0\0\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377\216\0\0"
  "\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377"
  "\217\0\0\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377\216\0\0\0\0\2\0"
  "\0\0\377\274--\377\216\0\0\0\0\202\0\0\0\377\216\0\0\0\0"};


/* BdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (linear_pixdata)
#endif
#ifdef __GNUC__
static const guint8 linear_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 linear_pixdata[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "BdkP"
  /* length: header (24) + pixel_data (323) */
  "\0\0\1["
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\216\0\0\0\0\202\0\0\0\377\216\0\0\0\0\2\202AA\377\0\0\0\377\216\0\0"
  "\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377"
  "\206\0\0\0\0\3\177\177\177\377\0\0\0\377\177\177\177\377\205\0\0\0\0"
  "\1\377\0\0\377\207\0\0\0\0\203\0\0\0\377\205\0\0\0\0\1\377\0\0\377\207"
  "\0\0\0\0\3\202AA\377\0\0\0\377\202AA\377\204\0\0\0\0\1\377\0\0\377\210"
  "\0\0\0\0\3\377\0\0\377\0\0\0\0\377\0\0\377\204\0\0\0\0\1\377\0\0\377"
  "\207\0\0\0\0\1\377\0\0\377\203\0\0\0\0\1\377\0\0\377\202\0\0\0\0\1\377"
  "\0\0\377\210\0\0\0\0\1\377\0\0\377\203\0\0\0\0\1\377\0\0\377\202\0\0"
  "\0\0\1\377\0\0\377\207\0\0\0\0\1\377\0\0\377\205\0\0\0\0\3\377\0\0\377"
  "\0\0\0\377\202AA\377\207\0\0\0\0\1\377\0\0\377\205\0\0\0\0\203\0\0\0"
  "\377\206\0\0\0\0\1\377\0\0\377\206\0\0\0\0\3\177\177\177\377\0\0\0\377"
  "\177\177\177\377\205\0\0\0\0\2\0\0\0\377\202AA\377\216\0\0\0\0\202\0"
  "\0\0\377\216\0\0\0\0"};


/* BdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (free_pixdata)
#endif
#ifdef __GNUC__
static const guint8 free_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 free_pixdata[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "BdkP"
  /* length: header (24) + pixel_data (204) */
  "\0\0\0\344"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\246\0\0\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377\220\0\0\0\0\1\377"
  "\0\0\377\217\0\0\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377\220\0\0"
  "\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377\217\0\0\0\0\1\377\0\0\377"
  "\214\0\0\0\0\1\377\0\0\377\203\0\0\0\0\2\377\0\0\377\0\0\0\0\205\377"
  "\0\0\377\204\0\0\0\0\1\377\0\0\377\204\0\0\0\0\1\377\0\0\377\211\0\0"
  "\0\0\1\377\0\0\377\205\0\0\0\0\1\377\0\0\377\210\0\0\0\0\1\377\0\0\377"
  "\207\0\0\0\0\1\377\0\0\377\206\0\0\0\0\1\377\0\0\377\210\0\0\0\0\1\377"
  "\0\0\377\205\0\0\0\0\1\377\0\0\377\217\0\0\0\0"};


/* BdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (gamma_pixdata)
#endif
#ifdef __GNUC__
static const guint8 gamma_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 gamma_pixdata[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "BdkP"
  /* length: header (24) + pixel_data (218) */
  "\0\0\0\362"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\264\0\0\0\0\2\0\0\0\377^^^\377\202\0\0\0\0\3\214\214\214\377\0\0\0\377"
  "\214\214\214\377\211\0\0\0\0\7FFF\377\27\27\27\377\273\273\273\377\0"
  "\0\0\0uuu\377\27\27\27\377\244\244\244\377\212\0\0\0\0\3uuu\377\214\214"
  "\214\377\0\0\0\0\202FFF\377\214\0\0\0\0\4\0\0\0\377\0\0\0\0\0\0\0\377"
  "\214\214\214\377\214\0\0\0\0\3FFF\377\0\0\0\0FFF\377\215\0\0\0\0\3FF"
  "F\377\27\27\27\377\214\214\214\377\215\0\0\0\0\2\244\244\244\377\0\0"
  "\0\377\216\0\0\0\0\2uuu\377^^^\377\216\0\0\0\0\2///\377\0\0\0\377\216"
  "\0\0\0\0\202\0\0\0\377\216\0\0\0\0\2\0\0\0\377///\377\250\0\0\0\0"};


/* BdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (reset_pixdata)
#endif
#ifdef __GNUC__
static const guint8 reset_pixdata[] __attribute__ ((__aligned__ (4))) = 
#else
static const guint8 reset_pixdata[] = 
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "BdkP"
  /* length: header (24) + pixel_data (173) */
  "\0\0\0\305"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\216\0\0\0\0\202\0\0\0\377\216\0\0\0\0\2\202AA\377\0\0\0\377\215\0\0"
  "\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377"
  "\216\0\0\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377\216\0\0\0\0\1\377"
  "\0\0\377\216\0\0\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377\216\0\0"
  "\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377\216\0\0\0\0\1\377\0\0\377"
  "\216\0\0\0\0\1\377\0\0\377\215\0\0\0\0\2\0\0\0\377\202AA\377\216\0\0"
  "\0\0\202\0\0\0\377\216\0\0\0\0"};

G_DEFINE_TYPE (BtkGammaCurve, btk_gamma_curve, BTK_TYPE_VBOX)

static void
btk_gamma_curve_class_init (BtkGammaCurveClass *class)
{
  BtkObjectClass *object_class;

  object_class = (BtkObjectClass *) class;
  object_class->destroy = btk_gamma_curve_destroy;
}

static void
btk_gamma_curve_init (BtkGammaCurve *curve)
{
  BtkWidget *vbox;
  int i;

  curve->gamma = 1.0;

  curve->table = btk_table_new (1, 2, FALSE);
  btk_table_set_col_spacings (BTK_TABLE (curve->table), 3);
  btk_container_add (BTK_CONTAINER (curve), curve->table);

  curve->curve = btk_curve_new ();
  g_signal_connect (curve->curve, "curve-type-changed",
		    G_CALLBACK (curve_type_changed_callback), curve);
  btk_table_attach_defaults (BTK_TABLE (curve->table), curve->curve, 0, 1, 0, 1);

  vbox = btk_vbox_new (/* homogeneous */ FALSE, /* spacing */ 3);
  btk_table_attach (BTK_TABLE (curve->table), vbox, 1, 2, 0, 1, 0, 0, 0, 0);

  /* toggle buttons: */
  for (i = 0; i < 3; ++i)
    {
      curve->button[i] = btk_toggle_button_new ();
      g_object_set_data (B_OBJECT (curve->button[i]), I_("_BtkGammaCurveIndex"),
			 GINT_TO_POINTER (i));
      btk_container_add (BTK_CONTAINER (vbox), curve->button[i]);
      g_signal_connect (curve->button[i], "realize",
			G_CALLBACK (button_realize_callback), NULL);
      g_signal_connect (curve->button[i], "toggled",
			G_CALLBACK (button_toggled_callback), curve);
      btk_widget_show (curve->button[i]);
    }

  /* push buttons: */
  for (i = 3; i < 5; ++i)
    {
      curve->button[i] = btk_button_new ();
      g_object_set_data (B_OBJECT (curve->button[i]), I_("_BtkGammaCurveIndex"),
			 GINT_TO_POINTER (i));
      btk_container_add (BTK_CONTAINER (vbox), curve->button[i]);
      g_signal_connect (curve->button[i], "realize",
			G_CALLBACK (button_realize_callback), NULL);
      g_signal_connect (curve->button[i], "clicked",
			G_CALLBACK (button_clicked_callback), curve);
      btk_widget_show (curve->button[i]);
    }

  btk_widget_show (vbox);
  btk_widget_show (curve->table);
  btk_widget_show (curve->curve);
}

static void
button_realize_callback (BtkWidget *w)
{
  BtkWidget *image;
  struct {
    const guint8 *stream;
    gint length;
  } streams[5] = {
    { linear_pixdata, sizeof (linear_pixdata) },
    { spline_pixdata, sizeof (spline_pixdata) },
    { free_pixdata, sizeof (free_pixdata) },
    { gamma_pixdata, sizeof (gamma_pixdata) },
    { reset_pixdata, sizeof (reset_pixdata) }
  };
  BdkPixdata pixdata;
  BdkPixbuf *pixbuf;
  int i;

  i = GPOINTER_TO_INT (g_object_get_data (B_OBJECT (w), "_BtkGammaCurveIndex"));
  bdk_pixdata_deserialize (&pixdata, streams[i].length, streams[i].stream, NULL);
  pixbuf = bdk_pixbuf_from_pixdata (&pixdata, TRUE, NULL);
  image = btk_image_new_from_pixbuf (pixbuf);
  btk_container_add (BTK_CONTAINER (w), image);
  btk_widget_show (image);

  g_object_unref (pixbuf);
}

static void
button_toggled_callback (BtkWidget *w, gpointer data)
{
  BtkGammaCurve *c = data;
  BtkCurveType type;
  int active, i;

  if (!BTK_TOGGLE_BUTTON (w)->active)
    return;

  active = GPOINTER_TO_INT (g_object_get_data (B_OBJECT (w), "_BtkGammaCurveIndex"));

  for (i = 0; i < 3; ++i)
    if ((i != active) && BTK_TOGGLE_BUTTON (c->button[i])->active)
      break;

  if (i < 3)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (c->button[i]), FALSE);

  switch (active)
    {
    case 0:  type = BTK_CURVE_TYPE_SPLINE; break;
    case 1:  type = BTK_CURVE_TYPE_LINEAR; break;
    default: type = BTK_CURVE_TYPE_FREE; break;
    }
  btk_curve_set_curve_type (BTK_CURVE (c->curve), type);
}

static void
gamma_cancel_callback (BtkWidget *w, gpointer data)
{
  BtkGammaCurve *c = data;

  btk_widget_destroy (c->gamma_dialog);
}

static void
gamma_ok_callback (BtkWidget *w, gpointer data)
{
  BtkGammaCurve *c = data;
  const gchar *start;
  gchar *end;
  gfloat v;

  start = btk_entry_get_text (BTK_ENTRY (c->gamma_text));
  if (start)
    {
      v = g_strtod (start, &end);
      if (end > start && v > 0.0)
	c->gamma = v;
    }
  btk_curve_set_gamma (BTK_CURVE (c->curve), c->gamma);
  gamma_cancel_callback (w, data);
}

static void
button_clicked_callback (BtkWidget *w, gpointer data)
{
  BtkGammaCurve *c = data;
  int active;

  active = GPOINTER_TO_INT (g_object_get_data (B_OBJECT (w), "_BtkGammaCurveIndex"));
  if (active == 3)
    {
      /* set gamma */
      if (c->gamma_dialog)
	return;
      else
	{
	  BtkWidget *vbox, *hbox, *label, *button;
	  gchar buf[64];
	  
	  c->gamma_dialog = btk_dialog_new ();
	  btk_window_set_screen (BTK_WINDOW (c->gamma_dialog),
				 btk_widget_get_screen (w));
	  btk_window_set_title (BTK_WINDOW (c->gamma_dialog), _("Gamma"));
	  g_object_add_weak_pointer (B_OBJECT (c->gamma_dialog),
				     (gpointer *)&c->gamma_dialog);
	  
	  vbox = BTK_DIALOG (c->gamma_dialog)->vbox;
	  
	  hbox = btk_hbox_new (/* homogeneous */ FALSE, 0);
	  btk_box_pack_start (BTK_BOX (vbox), hbox, TRUE, TRUE, 2);
	  btk_widget_show (hbox);
	  
	  label = btk_label_new_with_mnemonic (_("_Gamma value"));
	  btk_box_pack_start (BTK_BOX (hbox), label, FALSE, FALSE, 2);
	  btk_widget_show (label);
	  
	  sprintf (buf, "%g", c->gamma);
	  c->gamma_text = btk_entry_new ();
          btk_label_set_mnemonic_widget (BTK_LABEL (label), c->gamma_text);
	  btk_entry_set_text (BTK_ENTRY (c->gamma_text), buf);
	  btk_box_pack_start (BTK_BOX (hbox), c->gamma_text, TRUE, TRUE, 2);
	  btk_widget_show (c->gamma_text);
	  
	  /* fill in action area: */
	  hbox = BTK_DIALOG (c->gamma_dialog)->action_area;

          button = btk_button_new_from_stock (BTK_STOCK_CANCEL);
	  g_signal_connect (button, "clicked",
			    G_CALLBACK (gamma_cancel_callback), c);
	  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
	  btk_widget_show (button);
	  
          button = btk_button_new_from_stock (BTK_STOCK_OK);
	  btk_widget_set_can_default (button, TRUE);
	  g_signal_connect (button, "clicked",
			    G_CALLBACK (gamma_ok_callback), c);
	  btk_box_pack_start (BTK_BOX (hbox), button, TRUE, TRUE, 0);
	  btk_widget_grab_default (button);
	  btk_widget_show (button);
	  
	  btk_widget_show (c->gamma_dialog);
	}
    }
  else
    {
      /* reset */
      btk_curve_reset (BTK_CURVE (c->curve));
    }
}

static void
curve_type_changed_callback (BtkWidget *w, gpointer data)
{
  BtkGammaCurve *c = data;
  BtkCurveType new_type;
  int active;

  new_type = BTK_CURVE (w)->curve_type;
  switch (new_type)
    {
    case BTK_CURVE_TYPE_SPLINE: active = 0; break;
    case BTK_CURVE_TYPE_LINEAR: active = 1; break;
    default:		        active = 2; break;
    }
  if (!BTK_TOGGLE_BUTTON (c->button[active])->active)
    btk_toggle_button_set_active (BTK_TOGGLE_BUTTON (c->button[active]), TRUE);
}

BtkWidget*
btk_gamma_curve_new (void)
{
  return g_object_new (BTK_TYPE_GAMMA_CURVE, NULL);
}

static void
btk_gamma_curve_destroy (BtkObject *object)
{
  BtkGammaCurve *c = BTK_GAMMA_CURVE (object);

  if (c->gamma_dialog)
    btk_widget_destroy (c->gamma_dialog);

  BTK_OBJECT_CLASS (btk_gamma_curve_parent_class)->destroy (object);
}

#define __BTK_GAMMA_CURVE_C__
#include "btkaliasdef.c"
