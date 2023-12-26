/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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
 * Modified by the BTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the BTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BTK+ at ftp://ftp.btk.org/pub/btk/. 
 */

#include "config.h"
#include <math.h>
#include <string.h>

#include "bdkconfig.h"
#include "bdk/bdkkeysyms.h"
#include "btkcolorsel.h"
#include "btkhsv.h"
#include "btkwindow.h"
#include "btkselection.h"
#include "btkdnd.h"
#include "btkdrawingarea.h"
#include "btkhbox.h"
#include "btkhbbox.h"
#include "btkrc.h"
#include "btkframe.h"
#include "btktable.h"
#include "btklabel.h"
#include "btkmarshalers.h"
#include "btkimage.h"
#include "btkspinbutton.h"
#include "btkrange.h"
#include "btkhscale.h"
#include "btkentry.h"
#include "btkbutton.h"
#include "btkhseparator.h"
#include "btkinvisible.h"
#include "btkmenuitem.h"
#include "btkmain.h"
#include "btksettings.h"
#include "btkstock.h"
#include "btkaccessible.h"
#include "btkprivate.h"
#include "btkintl.h"
#include "btkalias.h"

/* Keep it in sync with btksettings.c:default_color_palette */
#define DEFAULT_COLOR_PALETTE   "black:white:gray50:red:purple:blue:light blue:green:yellow:orange:lavender:brown:goldenrod4:dodger blue:pink:light green:gray10:gray30:gray75:gray90"

/* Number of elements in the custom palatte */
#define BTK_CUSTOM_PALETTE_WIDTH 10
#define BTK_CUSTOM_PALETTE_HEIGHT 2

#define CUSTOM_PALETTE_ENTRY_WIDTH   20
#define CUSTOM_PALETTE_ENTRY_HEIGHT  20

/* The cursor for the dropper */
#define DROPPER_WIDTH 17
#define DROPPER_HEIGHT 17
#define DROPPER_STRIDE 4
#define DROPPER_X_HOT 2
#define DROPPER_Y_HOT 16

#define SAMPLE_WIDTH  64
#define SAMPLE_HEIGHT 28
#define CHECK_SIZE 16  
#define BIG_STEP 20

/* Conversion between 0->1 double and and guint16. See
 * scale_round() below for more general conversions
 */
#define SCALE(i) (i / 65535.)
#define UNSCALE(d) ((guint16)(d * 65535 + 0.5))
#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)

enum {
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_HAS_PALETTE,
  PROP_HAS_OPACITY_CONTROL,
  PROP_CURRENT_COLOR,
  PROP_CURRENT_ALPHA
};

enum {
  COLORSEL_RED = 0,
  COLORSEL_GREEN = 1,
  COLORSEL_BLUE = 2,
  COLORSEL_OPACITY = 3,
  COLORSEL_HUE,
  COLORSEL_SATURATION,
  COLORSEL_VALUE,
  COLORSEL_NUM_CHANNELS
};

typedef struct _ColorSelectionPrivate ColorSelectionPrivate;

struct _ColorSelectionPrivate
{
  guint has_opacity : 1;
  guint has_palette : 1;
  guint changing : 1;
  guint default_set : 1;
  guint default_alpha_set : 1;
  guint has_grab : 1;
  
  gdouble color[COLORSEL_NUM_CHANNELS];
  gdouble old_color[COLORSEL_NUM_CHANNELS];
  
  BtkWidget *triangle_colorsel;
  BtkWidget *hue_spinbutton;
  BtkWidget *sat_spinbutton;
  BtkWidget *val_spinbutton;
  BtkWidget *red_spinbutton;
  BtkWidget *green_spinbutton;
  BtkWidget *blue_spinbutton;
  BtkWidget *opacity_slider;
  BtkWidget *opacity_label;
  BtkWidget *opacity_entry;
  BtkWidget *palette_frame;
  BtkWidget *hex_entry;
  
  /* The Palette code */
  BtkWidget *custom_palette [BTK_CUSTOM_PALETTE_WIDTH][BTK_CUSTOM_PALETTE_HEIGHT];
  
  /* The color_sample stuff */
  BtkWidget *sample_area;
  BtkWidget *old_sample;
  BtkWidget *cur_sample;
  BtkWidget *colorsel;

  /* Window for grabbing on */
  BtkWidget *dropper_grab_widget;
  guint32    grab_time;

  /* Connection to settings */
  gulong settings_connection;
};


static void btk_color_selection_destroy		(BtkObject		 *object);
static void btk_color_selection_finalize        (GObject		 *object);
static void update_color			(BtkColorSelection	 *colorsel);
static void btk_color_selection_set_property    (GObject                 *object,
					         guint                    prop_id,
					         const GValue            *value,
					         GParamSpec              *pspec);
static void btk_color_selection_get_property    (GObject                 *object,
					         guint                    prop_id,
					         GValue                  *value,
					         GParamSpec              *pspec);

static void btk_color_selection_realize         (BtkWidget               *widget);
static void btk_color_selection_unrealize       (BtkWidget               *widget);
static void btk_color_selection_show_all        (BtkWidget               *widget);
static gboolean btk_color_selection_grab_broken (BtkWidget               *widget,
						 BdkEventGrabBroken      *event);

static void     btk_color_selection_set_palette_color   (BtkColorSelection *colorsel,
                                                         gint               index,
                                                         BdkColor          *color);
static void     set_focus_line_attributes               (BtkWidget         *drawing_area,
							 bairo_t           *cr,
							 gint              *focus_width);
static void     default_noscreen_change_palette_func    (const BdkColor    *colors,
							 gint               n_colors);
static void     default_change_palette_func             (BdkScreen	   *screen,
							 const BdkColor    *colors,
							 gint               n_colors);
static void     make_control_relations                  (BatkObject         *batk_obj,
                                                         BtkWidget         *widget);
static void     make_all_relations                      (BatkObject         *batk_obj,
                                                         ColorSelectionPrivate *priv);

static void 	hsv_changed                             (BtkWidget         *hsv,
							 gpointer           data);
static void 	get_screen_color                        (BtkWidget         *button);
static void 	adjustment_changed                      (BtkAdjustment     *adjustment,
							 gpointer           data);
static void 	opacity_entry_changed                   (BtkWidget 	   *opacity_entry,
							 gpointer  	    data);
static void 	hex_changed                             (BtkWidget 	   *hex_entry,
							 gpointer  	    data);
static gboolean hex_focus_out                           (BtkWidget     	   *hex_entry, 
							 BdkEventFocus 	   *event,
							 gpointer      	    data);
static void 	color_sample_new                        (BtkColorSelection *colorsel);
static void 	make_label_spinbutton     		(BtkColorSelection *colorsel,
	    				  		 BtkWidget        **spinbutton,
	    				  		 gchar             *text,
	    				  		 BtkWidget         *table,
	    				  		 gint               i,
	    				  		 gint               j,
	    				  		 gint               channel_type,
	    				  		 const gchar       *tooltip);
static void 	make_palette_frame                      (BtkColorSelection *colorsel,
							 BtkWidget         *table,
							 gint               i,
							 gint               j);
static void 	set_selected_palette                    (BtkColorSelection *colorsel,
							 int                x,
							 int                y);
static void 	set_focus_line_attributes               (BtkWidget 	   *drawing_area,
							 bairo_t   	   *cr,
							 gint      	   *focus_width);
static gboolean mouse_press 		     	       	(BtkWidget         *invisible,
                            		     	       	 BdkEventButton    *event,
                            		     	       	 gpointer           data);
static void  palette_change_notify_instance (GObject    *object,
					     GParamSpec *pspec,
					     gpointer    data);
static void update_palette (BtkColorSelection *colorsel);
static void shutdown_eyedropper (BtkWidget *widget);

static guint color_selection_signals[LAST_SIGNAL] = { 0 };

static BtkColorSelectionChangePaletteFunc noscreen_change_palette_hook = default_noscreen_change_palette_func;
static BtkColorSelectionChangePaletteWithScreenFunc change_palette_hook = default_change_palette_func;

static const guchar dropper_bits[] = {
  0xff, 0x8f, 0x01, 0x00,  0xff, 0x77, 0x01, 0x00,
  0xff, 0xfb, 0x00, 0x00,  0xff, 0xf8, 0x00, 0x00,
  0x7f, 0xff, 0x00, 0x00,  0xff, 0x7e, 0x01, 0x00,
  0xff, 0x9d, 0x01, 0x00,  0xff, 0xd8, 0x01, 0x00,
  0x7f, 0xd4, 0x01, 0x00,  0x3f, 0xee, 0x01, 0x00,
  0x1f, 0xff, 0x01, 0x00,  0x8f, 0xff, 0x01, 0x00,
  0xc7, 0xff, 0x01, 0x00,  0xe3, 0xff, 0x01, 0x00,
  0xf3, 0xff, 0x01, 0x00,  0xfd, 0xff, 0x01, 0x00,
  0xff, 0xff, 0x01, 0x00 };

static const guchar dropper_mask[] = {
  0x00, 0x70, 0x00, 0x00,  0x00, 0xf8, 0x00, 0x00,
  0x00, 0xfc, 0x01, 0x00,  0x00, 0xff, 0x01, 0x00,
  0x80, 0xff, 0x01, 0x00,  0x00, 0xff, 0x00, 0x00,
  0x00, 0x7f, 0x00, 0x00,  0x80, 0x3f, 0x00, 0x00,
  0xc0, 0x3f, 0x00, 0x00,  0xe0, 0x13, 0x00, 0x00,
  0xf0, 0x01, 0x00, 0x00,  0xf8, 0x00, 0x00, 0x00,
  0x7c, 0x00, 0x00, 0x00,  0x3e, 0x00, 0x00, 0x00,
  0x1e, 0x00, 0x00, 0x00,  0x0d, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00 };

G_DEFINE_TYPE (BtkColorSelection, btk_color_selection, BTK_TYPE_VBOX)

static void
btk_color_selection_class_init (BtkColorSelectionClass *klass)
{
  GObjectClass *bobject_class;
  BtkObjectClass *object_class;
  BtkWidgetClass *widget_class;
  
  bobject_class = G_OBJECT_CLASS (klass);
  bobject_class->finalize = btk_color_selection_finalize;
  bobject_class->set_property = btk_color_selection_set_property;
  bobject_class->get_property = btk_color_selection_get_property;

  object_class = BTK_OBJECT_CLASS (klass);
  object_class->destroy = btk_color_selection_destroy;
  
  widget_class = BTK_WIDGET_CLASS (klass);
  widget_class->realize = btk_color_selection_realize;
  widget_class->unrealize = btk_color_selection_unrealize;
  widget_class->show_all = btk_color_selection_show_all;
  widget_class->grab_broken_event = btk_color_selection_grab_broken;
  
  g_object_class_install_property (bobject_class,
                                   PROP_HAS_OPACITY_CONTROL,
                                   g_param_spec_boolean ("has-opacity-control",
							 P_("Has Opacity Control"),
							 P_("Whether the color selector should allow setting opacity"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_HAS_PALETTE,
                                   g_param_spec_boolean ("has-palette",
							 P_("Has palette"),
							 P_("Whether a palette should be used"),
							 FALSE,
							 BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_CURRENT_COLOR,
                                   g_param_spec_boxed ("current-color",
                                                       P_("Current Color"),
                                                       P_("The current color"),
                                                       BDK_TYPE_COLOR,
                                                       BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
                                   PROP_CURRENT_ALPHA,
                                   g_param_spec_uint ("current-alpha",
						      P_("Current Alpha"),
						      P_("The current opacity value (0 fully transparent, 65535 fully opaque)"),
						      0, 65535, 65535,
						      BTK_PARAM_READWRITE));
  
  color_selection_signals[COLOR_CHANGED] =
    g_signal_new (I_("color-changed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BtkColorSelectionClass, color_changed),
		  NULL, NULL,
		  _btk_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_type_class_add_private (bobject_class, sizeof (ColorSelectionPrivate));
}

static void
btk_color_selection_init (BtkColorSelection *colorsel)
{
  BtkWidget *top_hbox;
  BtkWidget *top_right_vbox;
  BtkWidget *table, *label, *hbox, *frame, *vbox, *button;
  BtkAdjustment *adjust;
  BtkWidget *picker_image;
  gint i, j;
  ColorSelectionPrivate *priv;
  BatkObject *batk_obj;
  GList *focus_chain = NULL;
  
  btk_widget_push_composite_child ();

  priv = colorsel->private_data = G_TYPE_INSTANCE_GET_PRIVATE (colorsel, BTK_TYPE_COLOR_SELECTION, ColorSelectionPrivate);
  priv->changing = FALSE;
  priv->default_set = FALSE;
  priv->default_alpha_set = FALSE;
  
  top_hbox = btk_hbox_new (FALSE, 12);
  btk_box_pack_start (BTK_BOX (colorsel), top_hbox, FALSE, FALSE, 0);
  
  vbox = btk_vbox_new (FALSE, 6);
  priv->triangle_colorsel = btk_hsv_new ();
  g_signal_connect (priv->triangle_colorsel, "changed",
                    G_CALLBACK (hsv_changed), colorsel);
  btk_hsv_set_metrics (BTK_HSV (priv->triangle_colorsel), 174, 15);
  btk_box_pack_start (BTK_BOX (top_hbox), vbox, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (vbox), priv->triangle_colorsel, FALSE, FALSE, 0);
  btk_widget_set_tooltip_text (priv->triangle_colorsel,
                        _("Select the color you want from the outer ring. Select the darkness or lightness of that color using the inner triangle."));
  
  hbox = btk_hbox_new (FALSE, 6);
  btk_box_pack_end (BTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  frame = btk_frame_new (NULL);
  btk_widget_set_size_request (frame, -1, 30);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
  color_sample_new (colorsel);
  btk_container_add (BTK_CONTAINER (frame), priv->sample_area);
  btk_box_pack_start (BTK_BOX (hbox), frame, TRUE, TRUE, 0);
  
  button = btk_button_new ();

  btk_widget_set_events (button, BDK_POINTER_MOTION_MASK | BDK_POINTER_MOTION_HINT_MASK);
  g_object_set_data (G_OBJECT (button), I_("COLORSEL"), colorsel); 
  g_signal_connect (button, "clicked",
                    G_CALLBACK (get_screen_color), NULL);
  picker_image = btk_image_new_from_stock (BTK_STOCK_COLOR_PICKER, BTK_ICON_SIZE_BUTTON);
  btk_container_add (BTK_CONTAINER (button), picker_image);
  btk_widget_show (BTK_WIDGET (picker_image));
  btk_box_pack_end (BTK_BOX (hbox), button, FALSE, FALSE, 0);

  btk_widget_set_tooltip_text (button,
                        _("Click the eyedropper, then click a color anywhere on your screen to select that color."));
  
  top_right_vbox = btk_vbox_new (FALSE, 6);
  btk_box_pack_start (BTK_BOX (top_hbox), top_right_vbox, FALSE, FALSE, 0);
  table = btk_table_new (8, 6, FALSE);
  btk_box_pack_start (BTK_BOX (top_right_vbox), table, FALSE, FALSE, 0);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  
  make_label_spinbutton (colorsel, &priv->hue_spinbutton, _("_Hue:"), table, 0, 0, COLORSEL_HUE,
                         _("Position on the color wheel."));
  btk_spin_button_set_wrap (BTK_SPIN_BUTTON (priv->hue_spinbutton), TRUE);
  make_label_spinbutton (colorsel, &priv->sat_spinbutton, _("_Saturation:"), table, 0, 1, COLORSEL_SATURATION,
                         _("\"Deepness\" of the color."));
  make_label_spinbutton (colorsel, &priv->val_spinbutton, _("_Value:"), table, 0, 2, COLORSEL_VALUE,
                         _("Brightness of the color."));
  make_label_spinbutton (colorsel, &priv->red_spinbutton, _("_Red:"), table, 6, 0, COLORSEL_RED,
                         _("Amount of red light in the color."));
  make_label_spinbutton (colorsel, &priv->green_spinbutton, _("_Green:"), table, 6, 1, COLORSEL_GREEN,
                         _("Amount of green light in the color."));
  make_label_spinbutton (colorsel, &priv->blue_spinbutton, _("_Blue:"), table, 6, 2, COLORSEL_BLUE,
                         _("Amount of blue light in the color."));
  btk_table_attach_defaults (BTK_TABLE (table), btk_hseparator_new (), 0, 8, 3, 4); 

  priv->opacity_label = btk_label_new_with_mnemonic (_("Op_acity:")); 
  btk_misc_set_alignment (BTK_MISC (priv->opacity_label), 0.0, 0.5); 
  btk_table_attach_defaults (BTK_TABLE (table), priv->opacity_label, 0, 1, 4, 5); 
  adjust = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 255.0, 1.0, 1.0, 0.0)); 
  g_object_set_data (G_OBJECT (adjust), I_("COLORSEL"), colorsel); 
  priv->opacity_slider = btk_hscale_new (adjust);
  btk_widget_set_tooltip_text (priv->opacity_slider,
                        _("Transparency of the color."));
  btk_label_set_mnemonic_widget (BTK_LABEL (priv->opacity_label),
                                 priv->opacity_slider);
  btk_scale_set_draw_value (BTK_SCALE (priv->opacity_slider), FALSE);
  g_signal_connect (adjust, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (COLORSEL_OPACITY));
  btk_table_attach_defaults (BTK_TABLE (table), priv->opacity_slider, 1, 7, 4, 5); 
  priv->opacity_entry = btk_entry_new (); 
  btk_widget_set_tooltip_text (priv->opacity_entry,
                        _("Transparency of the color."));
  btk_widget_set_size_request (priv->opacity_entry, 40, -1); 

  g_signal_connect (priv->opacity_entry, "activate",
                    G_CALLBACK (opacity_entry_changed), colorsel);
  btk_table_attach_defaults (BTK_TABLE (table), priv->opacity_entry, 7, 8, 4, 5);
  
  label = btk_label_new_with_mnemonic (_("Color _name:"));
  btk_table_attach_defaults (BTK_TABLE (table), label, 0, 1, 5, 6);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  priv->hex_entry = btk_entry_new ();

  btk_label_set_mnemonic_widget (BTK_LABEL (label), priv->hex_entry);

  g_signal_connect (priv->hex_entry, "activate",
                    G_CALLBACK (hex_changed), colorsel);

  g_signal_connect (priv->hex_entry, "focus-out-event",
                    G_CALLBACK (hex_focus_out), colorsel);

  btk_widget_set_tooltip_text (priv->hex_entry,
                        _("You can enter an HTML-style hexadecimal color value, or simply a color name such as 'orange' in this entry."));
  
  btk_entry_set_width_chars (BTK_ENTRY (priv->hex_entry), 7);
  btk_table_attach_defaults (BTK_TABLE (table), priv->hex_entry, 1, 5, 5, 6);

  focus_chain = g_list_append (focus_chain, priv->hue_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->sat_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->val_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->red_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->green_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->blue_spinbutton);
  focus_chain = g_list_append (focus_chain, priv->opacity_slider);
  focus_chain = g_list_append (focus_chain, priv->opacity_entry);
  focus_chain = g_list_append (focus_chain, priv->hex_entry);
  btk_container_set_focus_chain (BTK_CONTAINER (table), focus_chain);
  g_list_free (focus_chain);

  /* Set up the palette */
  table = btk_table_new (BTK_CUSTOM_PALETTE_HEIGHT, BTK_CUSTOM_PALETTE_WIDTH, TRUE);
  btk_table_set_row_spacings (BTK_TABLE (table), 1);
  btk_table_set_col_spacings (BTK_TABLE (table), 1);
  for (i = 0; i < BTK_CUSTOM_PALETTE_WIDTH; i++)
    {
      for (j = 0; j < BTK_CUSTOM_PALETTE_HEIGHT; j++)
	{
	  make_palette_frame (colorsel, table, i, j);
	}
    }
  set_selected_palette (colorsel, 0, 0);
  priv->palette_frame = btk_vbox_new (FALSE, 6);
  label = btk_label_new_with_mnemonic (_("_Palette:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_box_pack_start (BTK_BOX (priv->palette_frame), label, FALSE, FALSE, 0);

  btk_label_set_mnemonic_widget (BTK_LABEL (label),
                                 priv->custom_palette[0][0]);
  
  btk_box_pack_end (BTK_BOX (top_right_vbox), priv->palette_frame, FALSE, FALSE, 0);
  btk_box_pack_start (BTK_BOX (priv->palette_frame), table, FALSE, FALSE, 0);
  
  btk_widget_show_all (top_hbox);

  /* hide unused stuff */
  
  if (priv->has_opacity == FALSE)
    {
      btk_widget_hide (priv->opacity_label);
      btk_widget_hide (priv->opacity_slider);
      btk_widget_hide (priv->opacity_entry);
    }
  
  if (priv->has_palette == FALSE)
    {
      btk_widget_hide (priv->palette_frame);
    }

  batk_obj = btk_widget_get_accessible (priv->triangle_colorsel);
  if (BTK_IS_ACCESSIBLE (batk_obj))
    {
      batk_object_set_name (batk_obj, _("Color Wheel"));
      batk_object_set_role (btk_widget_get_accessible (BTK_WIDGET (colorsel)), BATK_ROLE_COLOR_CHOOSER);
      make_all_relations (batk_obj, priv);
    } 

  btk_widget_pop_composite_child ();
}

/* GObject methods */
static void
btk_color_selection_finalize (GObject *object)
{
  G_OBJECT_CLASS (btk_color_selection_parent_class)->finalize (object);
}

static void
btk_color_selection_set_property (GObject         *object,
				  guint            prop_id,
				  const GValue    *value,
				  GParamSpec      *pspec)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (object);
  
  switch (prop_id)
    {
    case PROP_HAS_OPACITY_CONTROL:
      btk_color_selection_set_has_opacity_control (colorsel, 
						   g_value_get_boolean (value));
      break;
    case PROP_HAS_PALETTE:
      btk_color_selection_set_has_palette (colorsel, 
					   g_value_get_boolean (value));
      break;
    case PROP_CURRENT_COLOR:
      btk_color_selection_set_current_color (colorsel, g_value_get_boxed (value));
      break;
    case PROP_CURRENT_ALPHA:
      btk_color_selection_set_current_alpha (colorsel, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  
}

static void
btk_color_selection_get_property (GObject     *object,
				  guint        prop_id,
				  GValue      *value,
				  GParamSpec  *pspec)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (object);
  BdkColor color;
  
  switch (prop_id)
    {
    case PROP_HAS_OPACITY_CONTROL:
      g_value_set_boolean (value, btk_color_selection_get_has_opacity_control (colorsel));
      break;
    case PROP_HAS_PALETTE:
      g_value_set_boolean (value, btk_color_selection_get_has_palette (colorsel));
      break;
    case PROP_CURRENT_COLOR:
      btk_color_selection_get_current_color (colorsel, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_CURRENT_ALPHA:
      g_value_set_uint (value, btk_color_selection_get_current_alpha (colorsel));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* BtkObject methods */

static void
btk_color_selection_destroy (BtkObject *object)
{
  BtkColorSelection *cselection = BTK_COLOR_SELECTION (object);
  ColorSelectionPrivate *priv = cselection->private_data;

  if (priv->dropper_grab_widget)
    {
      btk_widget_destroy (priv->dropper_grab_widget);
      priv->dropper_grab_widget = NULL;
    }

  BTK_OBJECT_CLASS (btk_color_selection_parent_class)->destroy (object);
}

/* BtkWidget methods */

static void
btk_color_selection_realize (BtkWidget *widget)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (widget);
  ColorSelectionPrivate *priv = colorsel->private_data;
  BtkSettings *settings = btk_widget_get_settings (widget);

  priv->settings_connection =  g_signal_connect (settings,
						 "notify::btk-color-palette",
						 G_CALLBACK (palette_change_notify_instance),
						 widget);
  update_palette (colorsel);

  BTK_WIDGET_CLASS (btk_color_selection_parent_class)->realize (widget);
}

static void
btk_color_selection_unrealize (BtkWidget *widget)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (widget);
  ColorSelectionPrivate *priv = colorsel->private_data;
  BtkSettings *settings = btk_widget_get_settings (widget);

  g_signal_handler_disconnect (settings, priv->settings_connection);

  BTK_WIDGET_CLASS (btk_color_selection_parent_class)->unrealize (widget);
}

/* We override show-all since we have internal widgets that
 * shouldn't be shown when you call show_all(), like the
 * palette and opacity sliders.
 */
static void
btk_color_selection_show_all (BtkWidget *widget)
{
  btk_widget_show (widget);
}

static gboolean 
btk_color_selection_grab_broken (BtkWidget          *widget,
				 BdkEventGrabBroken *event)
{
  shutdown_eyedropper (widget);

  return TRUE;
}

/*
 *
 * The Sample Color
 *
 */

static void color_sample_draw_sample (BtkColorSelection *colorsel, int which);
static void color_sample_update_samples (BtkColorSelection *colorsel);

static void
set_color_internal (BtkColorSelection *colorsel,
		    gdouble           *color)
{
  ColorSelectionPrivate *priv;
  gint i;
  
  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->color[COLORSEL_RED] = color[0];
  priv->color[COLORSEL_GREEN] = color[1];
  priv->color[COLORSEL_BLUE] = color[2];
  priv->color[COLORSEL_OPACITY] = color[3];
  btk_rgb_to_hsv (priv->color[COLORSEL_RED],
		  priv->color[COLORSEL_GREEN],
		  priv->color[COLORSEL_BLUE],
		  &priv->color[COLORSEL_HUE],
		  &priv->color[COLORSEL_SATURATION],
		  &priv->color[COLORSEL_VALUE]);
  if (priv->default_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
	priv->old_color[i] = priv->color[i];
    }
  priv->default_set = TRUE;
  priv->default_alpha_set = TRUE;
  update_color (colorsel);
}

static void
set_color_icon (BdkDragContext *context,
		gdouble        *colors)
{
  BdkPixbuf *pixbuf;
  guint32 pixel;

  pixbuf = bdk_pixbuf_new (BDK_COLORSPACE_RGB, FALSE,
			   8, 48, 32);

  pixel = (((UNSCALE (colors[COLORSEL_RED])   & 0xff00) << 16) |
	   ((UNSCALE (colors[COLORSEL_GREEN]) & 0xff00) << 8) |
	   ((UNSCALE (colors[COLORSEL_BLUE])  & 0xff00)));

  bdk_pixbuf_fill (pixbuf, pixel);
  
  btk_drag_set_icon_pixbuf (context, pixbuf, -2, -2);
  g_object_unref (pixbuf);
}

static void
color_sample_drag_begin (BtkWidget      *widget,
			 BdkDragContext *context,
			 gpointer        data)
{
  BtkColorSelection *colorsel = data;
  ColorSelectionPrivate *priv;
  gdouble *colsrc;
  
  priv = colorsel->private_data;
  
  if (widget == priv->old_sample)
    colsrc = priv->old_color;
  else
    colsrc = priv->color;

  set_color_icon (context, colsrc);
}

static void
color_sample_drag_end (BtkWidget      *widget,
		       BdkDragContext *context,
		       gpointer        data)
{
  g_object_set_data (G_OBJECT (widget), I_("btk-color-selection-drag-window"), NULL);
}

static void
color_sample_drop_handle (BtkWidget        *widget,
			  BdkDragContext   *context,
			  gint              x,
			  gint              y,
			  BtkSelectionData *selection_data,
			  guint             info,
			  guint             time,
			  gpointer          data)
{
  BtkColorSelection *colorsel = data;
  ColorSelectionPrivate *priv;
  guint16 *vals;
  gdouble color[4];
  priv = colorsel->private_data;
  
  /* This is currently a guint16 array of the format:
   * R
   * G
   * B
   * opacity
   */
  
  if (selection_data->length < 0)
    return;
  
  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (selection_data->length != 8)
    {
      g_warning ("Received invalid color data\n");
      return;
    }
  
  vals = (guint16 *)selection_data->data;
  
  if (widget == priv->cur_sample)
    {
      color[0] = (gdouble)vals[0] / 0xffff;
      color[1] = (gdouble)vals[1] / 0xffff;
      color[2] = (gdouble)vals[2] / 0xffff;
      color[3] = (gdouble)vals[3] / 0xffff;
      
      set_color_internal (colorsel, color);
    }
}

static void
color_sample_drag_handle (BtkWidget        *widget,
			  BdkDragContext   *context,
			  BtkSelectionData *selection_data,
			  guint             info,
			  guint             time,
			  gpointer          data)
{
  BtkColorSelection *colorsel = data;
  ColorSelectionPrivate *priv;
  guint16 vals[4];
  gdouble *colsrc;
  
  priv = colorsel->private_data;
  
  if (widget == priv->old_sample)
    colsrc = priv->old_color;
  else
    colsrc = priv->color;
  
  vals[0] = colsrc[COLORSEL_RED] * 0xffff;
  vals[1] = colsrc[COLORSEL_GREEN] * 0xffff;
  vals[2] = colsrc[COLORSEL_BLUE] * 0xffff;
  vals[3] = priv->has_opacity ? colsrc[COLORSEL_OPACITY] * 0xffff : 0xffff;
  
  btk_selection_data_set (selection_data,
			  bdk_atom_intern_static_string ("application/x-color"),
			  16, (guchar *)vals, 8);
}

/* which = 0 means draw old sample, which = 1 means draw new */
static void
color_sample_draw_sample (BtkColorSelection *colorsel, int which)
{
  BtkWidget *da;
  gint x, y, wid, heig, goff;
  ColorSelectionPrivate *priv;
  bairo_t *cr;
  
  g_return_if_fail (colorsel != NULL);
  priv = colorsel->private_data;
  
  g_return_if_fail (priv->sample_area != NULL);
  if (!btk_widget_is_drawable (priv->sample_area))
    return;

  if (which == 0)
    {
      da = priv->old_sample;
      goff = 0;
    }
  else
    {
      da = priv->cur_sample;
      goff =  priv->old_sample->allocation.width % 32;
    }

  cr = bdk_bairo_create (da->window);
  
  wid = da->allocation.width;
  heig = da->allocation.height;

  /* Below needs tweaking for non-power-of-two */  
  
  if (priv->has_opacity)
    {
      /* Draw checks in background */

      bairo_set_source_rgb (cr, 0.5, 0.5, 0.5);
      bairo_rectangle (cr, 0, 0, wid, heig);
      bairo_fill (cr);

      bairo_set_source_rgb (cr, 0.75, 0.75, 0.75);
      for (x = goff & -CHECK_SIZE; x < goff + wid; x += CHECK_SIZE)
	for (y = 0; y < heig; y += CHECK_SIZE)
	  if ((x / CHECK_SIZE + y / CHECK_SIZE) % 2 == 0)
	    bairo_rectangle (cr, x - goff, y, CHECK_SIZE, CHECK_SIZE);
      bairo_fill (cr);
    }

  if (which == 0)
    {
      bairo_set_source_rgba (cr,
			     priv->old_color[COLORSEL_RED], 
			     priv->old_color[COLORSEL_GREEN], 
			     priv->old_color[COLORSEL_BLUE],
			     priv->has_opacity ?
			        priv->old_color[COLORSEL_OPACITY] : 1.0);
    }
  else
    {
      bairo_set_source_rgba (cr,
			     priv->color[COLORSEL_RED], 
			     priv->color[COLORSEL_GREEN], 
			     priv->color[COLORSEL_BLUE],
			     priv->has_opacity ?
			       priv->color[COLORSEL_OPACITY] : 1.0);
    }

  bairo_rectangle (cr, 0, 0, wid, heig);
  bairo_fill (cr);

  bairo_destroy (cr);
}


static void
color_sample_update_samples (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv = colorsel->private_data;
  btk_widget_queue_draw (priv->old_sample);
  btk_widget_queue_draw (priv->cur_sample);
}

static gboolean
color_old_sample_expose (BtkWidget         *da,
			 BdkEventExpose    *event,
			 BtkColorSelection *colorsel)
{
  color_sample_draw_sample (colorsel, 0);
  return FALSE;
}


static gboolean
color_cur_sample_expose (BtkWidget         *da,
			 BdkEventExpose    *event,
			 BtkColorSelection *colorsel)
{
  color_sample_draw_sample (colorsel, 1);
  return FALSE;
}

static void
color_sample_setup_dnd (BtkColorSelection *colorsel, BtkWidget *sample)
{
  static const BtkTargetEntry targets[] = {
    { "application/x-color", 0 }
  };
  ColorSelectionPrivate *priv;
  priv = colorsel->private_data;
  
  btk_drag_source_set (sample,
		       BDK_BUTTON1_MASK | BDK_BUTTON3_MASK,
		       targets, 1,
		       BDK_ACTION_COPY | BDK_ACTION_MOVE);
  
  g_signal_connect (sample, "drag-begin",
		    G_CALLBACK (color_sample_drag_begin),
		    colorsel);
  if (sample == priv->cur_sample)
    {
      
      btk_drag_dest_set (sample,
			 BTK_DEST_DEFAULT_HIGHLIGHT |
			 BTK_DEST_DEFAULT_MOTION |
			 BTK_DEST_DEFAULT_DROP,
			 targets, 1,
			 BDK_ACTION_COPY);
      
      g_signal_connect (sample, "drag-end",
			G_CALLBACK (color_sample_drag_end),
			colorsel);
    }
  
  g_signal_connect (sample, "drag-data-get",
		    G_CALLBACK (color_sample_drag_handle),
		    colorsel);
  g_signal_connect (sample, "drag-data-received",
		    G_CALLBACK (color_sample_drop_handle),
		    colorsel);
  
}

static void
update_tooltips (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;

  priv = colorsel->private_data;

  if (priv->has_palette == TRUE)
    {
      btk_widget_set_tooltip_text (priv->old_sample,
                            _("The previously-selected color, for comparison to the color you're selecting now. You can drag this color to a palette entry, or select this color as current by dragging it to the other color swatch alongside."));

      btk_widget_set_tooltip_text (priv->cur_sample,
                            _("The color you've chosen. You can drag this color to a palette entry to save it for use in the future."));
    }
  else
    {
      btk_widget_set_tooltip_text (priv->old_sample,
                            _("The previously-selected color, for comparison to the color you're selecting now."));

      btk_widget_set_tooltip_text (priv->cur_sample,
                            _("The color you've chosen."));
    }
}

static void
color_sample_new (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;
  
  priv = colorsel->private_data;
  
  priv->sample_area = btk_hbox_new (FALSE, 0);
  priv->old_sample = btk_drawing_area_new ();
  priv->cur_sample = btk_drawing_area_new ();

  btk_box_pack_start (BTK_BOX (priv->sample_area), priv->old_sample,
		      TRUE, TRUE, 0);
  btk_box_pack_start (BTK_BOX (priv->sample_area), priv->cur_sample,
		      TRUE, TRUE, 0);
  
  g_signal_connect (priv->old_sample, "expose-event",
		    G_CALLBACK (color_old_sample_expose),
		    colorsel);
  g_signal_connect (priv->cur_sample, "expose-event",
		    G_CALLBACK (color_cur_sample_expose),
		    colorsel);
  
  color_sample_setup_dnd (colorsel, priv->old_sample);
  color_sample_setup_dnd (colorsel, priv->cur_sample);

  update_tooltips (colorsel);

  btk_widget_show_all (priv->sample_area);
}


/*
 *
 * The palette area code
 *
 */

static void
palette_get_color (BtkWidget *drawing_area, gdouble *color)
{
  gdouble *color_val;
  
  g_return_if_fail (color != NULL);
  
  color_val = g_object_get_data (G_OBJECT (drawing_area), "color_val");
  if (color_val == NULL)
    {
      /* Default to white for no good reason */
      color[0] = 1.0;
      color[1] = 1.0;
      color[2] = 1.0;
      color[3] = 1.0;
      return;
    }
  
  color[0] = color_val[0];
  color[1] = color_val[1];
  color[2] = color_val[2];
  color[3] = 1.0;
}

static void
palette_paint (BtkWidget    *drawing_area,
	       BdkRectangle *area,
	       gpointer      data)
{
  bairo_t *cr;
  gint focus_width;
    
  if (drawing_area->window == NULL)
    return;

  cr = bdk_bairo_create (drawing_area->window);

  bdk_bairo_set_source_color (cr, &drawing_area->style->bg[BTK_STATE_NORMAL]);
  bdk_bairo_rectangle (cr, area);
  bairo_fill (cr);
  
  if (btk_widget_has_focus (drawing_area))
    {
      set_focus_line_attributes (drawing_area, cr, &focus_width);

      bairo_rectangle (cr,
		       focus_width / 2., focus_width / 2.,
		       drawing_area->allocation.width - focus_width,
		       drawing_area->allocation.height - focus_width);
      bairo_stroke (cr);
    }

  bairo_destroy (cr);
}

static void
set_focus_line_attributes (BtkWidget *drawing_area,
			   bairo_t   *cr,
			   gint      *focus_width)
{
  gdouble color[4];
  gint8 *dash_list;
  
  btk_widget_style_get (drawing_area,
			"focus-line-width", focus_width,
			"focus-line-pattern", (gchar *)&dash_list,
			NULL);
      
  palette_get_color (drawing_area, color);

  if (INTENSITY (color[0], color[1], color[2]) > 0.5)
    bairo_set_source_rgb (cr, 0., 0., 0.);
  else
    bairo_set_source_rgb (cr, 1., 1., 1.);

  bairo_set_line_width (cr, *focus_width);

  if (dash_list[0])
    {
      gint n_dashes = strlen ((gchar *)dash_list);
      gdouble *dashes = g_new (gdouble, n_dashes);
      gdouble total_length = 0;
      gdouble dash_offset;
      gint i;

      for (i = 0; i < n_dashes; i++)
	{
	  dashes[i] = dash_list[i];
	  total_length += dash_list[i];
	}

      /* The dash offset here aligns the pattern to integer pixels
       * by starting the dash at the right side of the left border
       * Negative dash offsets in bairo don't work
       * (https://bugs.freedesktop.org/show_bug.cgi?id=2729)
       */
      dash_offset = - *focus_width / 2.;
      while (dash_offset < 0)
	dash_offset += total_length;
      
      bairo_set_dash (cr, dashes, n_dashes, dash_offset);
      g_free (dashes);
    }

  g_free (dash_list);
}

static void
palette_drag_begin (BtkWidget      *widget,
		    BdkDragContext *context,
		    gpointer        data)
{
  gdouble colors[4];
  
  palette_get_color (widget, colors);
  set_color_icon (context, colors);
}

static void
palette_drag_handle (BtkWidget        *widget,
		     BdkDragContext   *context,
		     BtkSelectionData *selection_data,
		     guint             info,
		     guint             time,
		     gpointer          data)
{
  guint16 vals[4];
  gdouble colsrc[4];
  
  palette_get_color (widget, colsrc);
  
  vals[0] = colsrc[COLORSEL_RED] * 0xffff;
  vals[1] = colsrc[COLORSEL_GREEN] * 0xffff;
  vals[2] = colsrc[COLORSEL_BLUE] * 0xffff;
  vals[3] = 0xffff;
  
  btk_selection_data_set (selection_data,
			  bdk_atom_intern_static_string ("application/x-color"),
			  16, (guchar *)vals, 8);
}

static void
palette_drag_end (BtkWidget      *widget,
		  BdkDragContext *context,
		  gpointer        data)
{
  g_object_set_data (G_OBJECT (widget), I_("btk-color-selection-drag-window"), NULL);
}

static BdkColor *
get_current_colors (BtkColorSelection *colorsel)
{
  BtkSettings *settings;
  BdkColor *colors = NULL;
  gint n_colors = 0;
  gchar *palette;

  settings = btk_widget_get_settings (BTK_WIDGET (colorsel));
  g_object_get (settings, "btk-color-palette", &palette, NULL);
  
  if (!btk_color_selection_palette_from_string (palette, &colors, &n_colors))
    {
      btk_color_selection_palette_from_string (DEFAULT_COLOR_PALETTE,
                                               &colors,
                                               &n_colors);
    }
  else
    {
      /* If there are less colors provided than the number of slots in the
       * color selection, we fill in the rest from the defaults.
       */
      if (n_colors < (BTK_CUSTOM_PALETTE_WIDTH * BTK_CUSTOM_PALETTE_HEIGHT))
	{
	  BdkColor *tmp_colors = colors;
	  gint tmp_n_colors = n_colors;
	  
	  btk_color_selection_palette_from_string (DEFAULT_COLOR_PALETTE,
                                                   &colors,
                                                   &n_colors);
	  memcpy (colors, tmp_colors, sizeof (BdkColor) * tmp_n_colors);

	  g_free (tmp_colors);
	}
    }

  /* make sure that we fill every slot */
  g_assert (n_colors == BTK_CUSTOM_PALETTE_WIDTH * BTK_CUSTOM_PALETTE_HEIGHT);
  g_free (palette);
  
  return colors;
}

/* Changes the model color */
static void
palette_change_color (BtkWidget         *drawing_area,
                      BtkColorSelection *colorsel,
                      gdouble           *color)
{
  gint x, y;
  ColorSelectionPrivate *priv;
  BdkColor bdk_color;
  BdkColor *current_colors;
  BdkScreen *screen;

  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (BTK_IS_DRAWING_AREA (drawing_area));
  
  priv = colorsel->private_data;
  
  bdk_color.red = UNSCALE (color[0]);
  bdk_color.green = UNSCALE (color[1]);
  bdk_color.blue = UNSCALE (color[2]);
  bdk_color.pixel = 0;

  x = 0;
  y = 0;			/* Quiet GCC */
  while (x < BTK_CUSTOM_PALETTE_WIDTH)
    {
      y = 0;
      while (y < BTK_CUSTOM_PALETTE_HEIGHT)
        {
          if (priv->custom_palette[x][y] == drawing_area)
            goto out;
          
          ++y;
        }

      ++x;
    }

 out:
  
  g_assert (x < BTK_CUSTOM_PALETTE_WIDTH || y < BTK_CUSTOM_PALETTE_HEIGHT);

  current_colors = get_current_colors (colorsel);
  current_colors[y * BTK_CUSTOM_PALETTE_WIDTH + x] = bdk_color;

  screen = btk_widget_get_screen (BTK_WIDGET (colorsel));
  if (change_palette_hook != default_change_palette_func)
    (* change_palette_hook) (screen, current_colors, 
			     BTK_CUSTOM_PALETTE_WIDTH * BTK_CUSTOM_PALETTE_HEIGHT);
  else if (noscreen_change_palette_hook != default_noscreen_change_palette_func)
    {
      if (screen != bdk_screen_get_default ())
	g_warning ("btk_color_selection_set_change_palette_hook used by widget is not on the default screen.");
      (* noscreen_change_palette_hook) (current_colors, 
					BTK_CUSTOM_PALETTE_WIDTH * BTK_CUSTOM_PALETTE_HEIGHT);
    }
  else
    (* change_palette_hook) (screen, current_colors, 
			     BTK_CUSTOM_PALETTE_WIDTH * BTK_CUSTOM_PALETTE_HEIGHT);

  g_free (current_colors);
}

/* Changes the view color */
static void
palette_set_color (BtkWidget         *drawing_area,
		   BtkColorSelection *colorsel,
		   gdouble           *color)
{
  gdouble *new_color = g_new (double, 4);
  BdkColor bdk_color;
  
  bdk_color.red = UNSCALE (color[0]);
  bdk_color.green = UNSCALE (color[1]);
  bdk_color.blue = UNSCALE (color[2]);

  btk_widget_modify_bg (drawing_area, BTK_STATE_NORMAL, &bdk_color);
  
  if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drawing_area), "color_set")) == 0)
    {
      static const BtkTargetEntry targets[] = {
	{ "application/x-color", 0 }
      };
      btk_drag_source_set (drawing_area,
			   BDK_BUTTON1_MASK | BDK_BUTTON3_MASK,
			   targets, 1,
			   BDK_ACTION_COPY | BDK_ACTION_MOVE);
      
      g_signal_connect (drawing_area, "drag-begin",
			G_CALLBACK (palette_drag_begin),
			colorsel);
      g_signal_connect (drawing_area, "drag-data-get",
			G_CALLBACK (palette_drag_handle),
			colorsel);
      
      g_object_set_data (G_OBJECT (drawing_area), I_("color_set"),
			 GINT_TO_POINTER (1));
    }

  new_color[0] = color[0];
  new_color[1] = color[1];
  new_color[2] = color[2];
  new_color[3] = 1.0;
  
  g_object_set_data_full (G_OBJECT (drawing_area), I_("color_val"), new_color, (GDestroyNotify)g_free);
}

static gboolean
palette_expose (BtkWidget      *drawing_area,
		BdkEventExpose *event,
		gpointer        data)
{
  if (drawing_area->window == NULL)
    return FALSE;
  
  palette_paint (drawing_area, &(event->area), data);
  
  return FALSE;
}

static void
popup_position_func (BtkMenu   *menu,
                     gint      *x,
                     gint      *y,
                     gboolean  *push_in,
                     gpointer	user_data)
{
  BtkWidget *widget;
  BtkRequisition req;      
  gint root_x, root_y;
  BdkScreen *screen;
  
  widget = BTK_WIDGET (user_data);
  
  g_return_if_fail (btk_widget_get_realized (widget));

  bdk_window_get_origin (widget->window, &root_x, &root_y);
  
  btk_widget_size_request (BTK_WIDGET (menu), &req);

  /* Put corner of menu centered on color cell */
  *x = root_x + widget->allocation.width / 2;
  *y = root_y + widget->allocation.height / 2;

  /* Ensure sanity */
  screen = btk_widget_get_screen (widget);
  *x = CLAMP (*x, 0, MAX (0, bdk_screen_get_width (screen) - req.width));
  *y = CLAMP (*y, 0, MAX (0, bdk_screen_get_height (screen) - req.height));
}

static void
save_color_selected (BtkWidget *menuitem,
                     gpointer   data)
{
  BtkColorSelection *colorsel;
  BtkWidget *drawing_area;
  ColorSelectionPrivate *priv;

  drawing_area = BTK_WIDGET (data);
  
  colorsel = BTK_COLOR_SELECTION (g_object_get_data (G_OBJECT (drawing_area),
                                                     "btk-color-sel"));

  priv = colorsel->private_data;
  
  palette_change_color (drawing_area, colorsel, priv->color);  
}

static void
do_popup (BtkColorSelection *colorsel,
          BtkWidget         *drawing_area,
          guint32            timestamp)
{
  BtkWidget *menu;
  BtkWidget *mi;
  
  g_object_set_data (G_OBJECT (drawing_area),
                     I_("btk-color-sel"),
                     colorsel);
  
  menu = btk_menu_new ();

  mi = btk_menu_item_new_with_mnemonic (_("_Save color here"));

  g_signal_connect (mi, "activate",
                    G_CALLBACK (save_color_selected),
                    drawing_area);
  
  btk_menu_shell_append (BTK_MENU_SHELL (menu), mi);

  btk_widget_show_all (mi);

  btk_menu_popup (BTK_MENU (menu), NULL, NULL,
                  popup_position_func, drawing_area,
                  3, timestamp);
}


static gboolean
palette_enter (BtkWidget        *drawing_area,
	       BdkEventCrossing *event,
	       gpointer        data)
{
  g_object_set_data (G_OBJECT (drawing_area),
		     I_("btk-colorsel-have-pointer"),
		     GUINT_TO_POINTER (TRUE));

  return FALSE;
}

static gboolean
palette_leave (BtkWidget        *drawing_area,
	       BdkEventCrossing *event,
	       gpointer        data)
{
  g_object_set_data (G_OBJECT (drawing_area),
		     I_("btk-colorsel-have-pointer"),
		     NULL);

  return FALSE;
}

static gboolean
palette_press (BtkWidget      *drawing_area,
	       BdkEventButton *event,
	       gpointer        data)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (data);

  btk_widget_grab_focus (drawing_area);

  if (_btk_button_event_triggers_context_menu (event))
    {
      do_popup (colorsel, drawing_area, event->time);
      return TRUE;
    }

  return FALSE;
}

static gboolean
palette_release (BtkWidget      *drawing_area,
		 BdkEventButton *event,
		 gpointer        data)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (data);

  btk_widget_grab_focus (drawing_area);

  if (event->button == 1 &&
      g_object_get_data (G_OBJECT (drawing_area),
			 "btk-colorsel-have-pointer") != NULL)
    {
      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (drawing_area), "color_set")) != 0)
        {
          gdouble color[4];
          palette_get_color (drawing_area, color);
          set_color_internal (colorsel, color);
        }
    }

  return FALSE;
}

static void
palette_drop_handle (BtkWidget        *widget,
		     BdkDragContext   *context,
		     gint              x,
		     gint              y,
		     BtkSelectionData *selection_data,
		     guint             info,
		     guint             time,
		     gpointer          data)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (data);
  guint16 *vals;
  gdouble color[4];
  
  if (selection_data->length < 0)
    return;
  
  /* We accept drops with the wrong format, since the KDE color
   * chooser incorrectly drops application/x-color with format 8.
   */
  if (selection_data->length != 8)
    {
      g_warning ("Received invalid color data\n");
      return;
    }
  
  vals = (guint16 *)selection_data->data;
  
  color[0] = (gdouble)vals[0] / 0xffff;
  color[1] = (gdouble)vals[1] / 0xffff;
  color[2] = (gdouble)vals[2] / 0xffff;
  color[3] = (gdouble)vals[3] / 0xffff;
  palette_change_color (widget, colorsel, color);
  set_color_internal (colorsel, color);
}

static gint
palette_activate (BtkWidget   *widget,
		  BdkEventKey *event,
		  gpointer     data)
{
  /* should have a drawing area subclass with an activate signal */
  if ((event->keyval == BDK_space) ||
      (event->keyval == BDK_Return) ||
      (event->keyval == BDK_ISO_Enter) ||
      (event->keyval == BDK_KP_Enter) ||
      (event->keyval == BDK_KP_Space))
    {
      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "color_set")) != 0)
        {
          gdouble color[4];
          palette_get_color (widget, color);
          set_color_internal (BTK_COLOR_SELECTION (data), color);
        }
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
palette_popup (BtkWidget *widget,
               gpointer   data)
{
  BtkColorSelection *colorsel = BTK_COLOR_SELECTION (data);

  do_popup (colorsel, widget, BDK_CURRENT_TIME);
  return TRUE;
}
               

static BtkWidget*
palette_new (BtkColorSelection *colorsel)
{
  BtkWidget *retval;
  ColorSelectionPrivate *priv;
  
  static const BtkTargetEntry targets[] = {
    { "application/x-color", 0 }
  };

  priv = colorsel->private_data;
  
  retval = btk_drawing_area_new ();

  btk_widget_set_can_focus (retval, TRUE);
  
  g_object_set_data (G_OBJECT (retval), I_("color_set"), GINT_TO_POINTER (0)); 
  btk_widget_set_events (retval, BDK_BUTTON_PRESS_MASK
                         | BDK_BUTTON_RELEASE_MASK
                         | BDK_EXPOSURE_MASK
                         | BDK_ENTER_NOTIFY_MASK
                         | BDK_LEAVE_NOTIFY_MASK);
  
  g_signal_connect (retval, "expose-event",
		    G_CALLBACK (palette_expose), colorsel);
  g_signal_connect (retval, "button-press-event",
		    G_CALLBACK (palette_press), colorsel);
  g_signal_connect (retval, "button-release-event",
		    G_CALLBACK (palette_release), colorsel);
  g_signal_connect (retval, "enter-notify-event",
		    G_CALLBACK (palette_enter), colorsel);
  g_signal_connect (retval, "leave-notify-event",
		    G_CALLBACK (palette_leave), colorsel);
  g_signal_connect (retval, "key-press-event",
		    G_CALLBACK (palette_activate), colorsel);
  g_signal_connect (retval, "popup-menu",
		    G_CALLBACK (palette_popup), colorsel);
  
  btk_drag_dest_set (retval,
		     BTK_DEST_DEFAULT_HIGHLIGHT |
		     BTK_DEST_DEFAULT_MOTION |
		     BTK_DEST_DEFAULT_DROP,
		     targets, 1,
		     BDK_ACTION_COPY);
  
  g_signal_connect (retval, "drag-end",
                    G_CALLBACK (palette_drag_end), NULL);
  g_signal_connect (retval, "drag-data-received",
                    G_CALLBACK (palette_drop_handle), colorsel);

  btk_widget_set_tooltip_text (retval,
                        _("Click this palette entry to make it the current color. To change this entry, drag a color swatch here or right-click it and select \"Save color here.\""));
  return retval;
}


/*
 *
 * The actual BtkColorSelection widget
 *
 */

static BdkCursor *
make_picker_cursor (BdkScreen *screen)
{
  BdkCursor *cursor;

  cursor = bdk_cursor_new_from_name (bdk_screen_get_display (screen),
				     "color-picker");

  if (!cursor)
    {
      BdkColor bg = { 0, 0xffff, 0xffff, 0xffff };
      BdkColor fg = { 0, 0x0000, 0x0000, 0x0000 };
      BdkWindow *window;
      BdkPixmap *pixmap, *mask;
      bairo_surface_t *image;
      bairo_t *cr;

      window = bdk_screen_get_root_window (screen);
      

      pixmap = bdk_pixmap_new (window, DROPPER_WIDTH, DROPPER_HEIGHT, 1);

      cr = bdk_bairo_create (pixmap);
      bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
      image = bairo_image_surface_create_for_data ((guchar *) dropper_bits,
                                                   BAIRO_FORMAT_A1,
                                                   DROPPER_WIDTH,
                                                   DROPPER_HEIGHT,
                                                   DROPPER_STRIDE);
      bairo_set_source_surface (cr, image, 0, 0);
      bairo_surface_destroy (image);
      bairo_paint (cr);
      bairo_destroy (cr);
      

      mask = bdk_pixmap_new (window, DROPPER_WIDTH, DROPPER_HEIGHT, 1);

      cr = bdk_bairo_create (mask);
      bairo_set_operator (cr, BAIRO_OPERATOR_SOURCE);
      image = bairo_image_surface_create_for_data ((guchar *) dropper_mask,
                                                   BAIRO_FORMAT_A1,
                                                   DROPPER_WIDTH,
                                                   DROPPER_HEIGHT,
                                                   DROPPER_STRIDE);
      bairo_set_source_surface (cr, image, 0, 0);
      bairo_surface_destroy (image);
      bairo_paint (cr);
      bairo_destroy (cr);
      
      cursor = bdk_cursor_new_from_pixmap (pixmap, mask, &fg, &bg,
					   DROPPER_X_HOT, DROPPER_Y_HOT);
      
      g_object_unref (pixmap);
      g_object_unref (mask);
    }
      
  return cursor;
}

static void
grab_color_at_mouse (BdkScreen *screen,
		     gint       x_root,
		     gint       y_root,
		     gpointer   data)
{
  BdkPixbuf *pixbuf;
  guchar *pixels;
  BtkColorSelection *colorsel = data;
  ColorSelectionPrivate *priv;
  BdkColor color;
  BdkWindow *root_window = bdk_screen_get_root_window (screen);
  
  priv = colorsel->private_data;
  
  pixbuf = bdk_pixbuf_get_from_drawable (NULL, root_window, NULL,
                                         x_root, y_root,
                                         0, 0,
                                         1, 1);
  if (!pixbuf)
    {
      gint x, y;
      BdkDisplay *display = bdk_screen_get_display (screen);
      BdkWindow *window = bdk_display_get_window_at_pointer (display, &x, &y);
      if (!window)
	return;
      pixbuf = bdk_pixbuf_get_from_drawable (NULL, window, NULL,
                                             x, y,
                                             0, 0,
                                             1, 1);
      if (!pixbuf)
	return;
    }
  pixels = bdk_pixbuf_get_pixels (pixbuf);
  color.red = pixels[0] * 0x101;
  color.green = pixels[1] * 0x101;
  color.blue = pixels[2] * 0x101;
  g_object_unref (pixbuf);

  priv->color[COLORSEL_RED] = SCALE (color.red);
  priv->color[COLORSEL_GREEN] = SCALE (color.green);
  priv->color[COLORSEL_BLUE] = SCALE (color.blue);
  
  btk_rgb_to_hsv (priv->color[COLORSEL_RED],
		  priv->color[COLORSEL_GREEN],
		  priv->color[COLORSEL_BLUE],
		  &priv->color[COLORSEL_HUE],
		  &priv->color[COLORSEL_SATURATION],
		  &priv->color[COLORSEL_VALUE]);

  update_color (colorsel);
}

static void
shutdown_eyedropper (BtkWidget *widget)
{
  BtkColorSelection *colorsel;
  ColorSelectionPrivate *priv;
  BdkDisplay *display = btk_widget_get_display (widget);

  colorsel = BTK_COLOR_SELECTION (widget);
  priv = colorsel->private_data;    

  if (priv->has_grab)
    {
      bdk_display_keyboard_ungrab (display, priv->grab_time);
      bdk_display_pointer_ungrab (display, priv->grab_time);
      btk_grab_remove (priv->dropper_grab_widget);

      priv->has_grab = FALSE;
    }
}

static void
mouse_motion (BtkWidget      *invisible,
	      BdkEventMotion *event,
	      gpointer        data)
{
  grab_color_at_mouse (bdk_event_get_screen ((BdkEvent *)event),
		       event->x_root, event->y_root, data); 
}

static gboolean
mouse_release (BtkWidget      *invisible,
	       BdkEventButton *event,
	       gpointer        data)
{
  /* BtkColorSelection *colorsel = data; */

  if (event->button != 1)
    return FALSE;

  grab_color_at_mouse (bdk_event_get_screen ((BdkEvent *)event),
		       event->x_root, event->y_root, data);

  shutdown_eyedropper (BTK_WIDGET (data));
  
  g_signal_handlers_disconnect_by_func (invisible,
					mouse_motion,
					data);
  g_signal_handlers_disconnect_by_func (invisible,
					mouse_release,
					data);

  return TRUE;
}

/* Helper Functions */

static gboolean
key_press (BtkWidget   *invisible,
           BdkEventKey *event,
           gpointer     data)
{  
  BdkDisplay *display = btk_widget_get_display (invisible);
  BdkScreen *screen = bdk_event_get_screen ((BdkEvent *)event);
  guint state = event->state & btk_accelerator_get_default_mod_mask ();
  gint x, y;
  gint dx, dy;

  bdk_display_get_pointer (display, NULL, &x, &y, NULL);

  dx = 0;
  dy = 0;

  switch (event->keyval) 
    {
    case BDK_space:
    case BDK_Return:
    case BDK_ISO_Enter:
    case BDK_KP_Enter:
    case BDK_KP_Space:
      grab_color_at_mouse (screen, x, y, data);
      /* fall through */

    case BDK_Escape:
      shutdown_eyedropper (data);
      
      g_signal_handlers_disconnect_by_func (invisible,
					    mouse_press,
					    data);
      g_signal_handlers_disconnect_by_func (invisible,
					    key_press,
					    data);
      
      return TRUE;

#if defined BDK_WINDOWING_X11 || defined BDK_WINDOWING_WIN32
    case BDK_Up:
    case BDK_KP_Up:
      dy = state == BDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case BDK_Down:
    case BDK_KP_Down:
      dy = state == BDK_MOD1_MASK ? BIG_STEP : 1;
      break;

    case BDK_Left:
    case BDK_KP_Left:
      dx = state == BDK_MOD1_MASK ? -BIG_STEP : -1;
      break;

    case BDK_Right:
    case BDK_KP_Right:
      dx = state == BDK_MOD1_MASK ? BIG_STEP : 1;
      break;
#endif

    default:
      return FALSE;
    }

  bdk_display_warp_pointer (display, screen, x + dx, y + dy);
  
  return TRUE;

}

static gboolean
mouse_press (BtkWidget      *invisible,
	     BdkEventButton *event,
	     gpointer        data)
{
  /* BtkColorSelection *colorsel = data; */
  
  if (event->type == BDK_BUTTON_PRESS &&
      event->button == 1)
    {
      g_signal_connect (invisible, "motion-notify-event",
                        G_CALLBACK (mouse_motion),
                        data);
      g_signal_connect (invisible, "button-release-event",
                        G_CALLBACK (mouse_release),
                        data);
      g_signal_handlers_disconnect_by_func (invisible,
					    mouse_press,
					    data);
      g_signal_handlers_disconnect_by_func (invisible,
					    key_press,
					    data);
      return TRUE;
    }

  return FALSE;
}

/* when the button is clicked */
static void
get_screen_color (BtkWidget *button)
{
  BtkColorSelection *colorsel = g_object_get_data (G_OBJECT (button), "COLORSEL");
  ColorSelectionPrivate *priv = colorsel->private_data;
  BdkScreen *screen = btk_widget_get_screen (BTK_WIDGET (button));
  BdkCursor *picker_cursor;
  BdkGrabStatus grab_status;
  BtkWidget *grab_widget, *toplevel;

  guint32 time = btk_get_current_event_time ();
  
  if (priv->dropper_grab_widget == NULL)
    {
      grab_widget = btk_window_new (BTK_WINDOW_POPUP);
      btk_window_set_screen (BTK_WINDOW (grab_widget), screen);
      btk_window_resize (BTK_WINDOW (grab_widget), 1, 1);
      btk_window_move (BTK_WINDOW (grab_widget), -100, -100);
      btk_widget_show (grab_widget);

      btk_widget_add_events (grab_widget,
                             BDK_BUTTON_RELEASE_MASK | BDK_BUTTON_PRESS_MASK | BDK_POINTER_MOTION_MASK);
      
      toplevel = btk_widget_get_toplevel (BTK_WIDGET (colorsel));
  
      if (BTK_IS_WINDOW (toplevel))
	{
	  if (BTK_WINDOW (toplevel)->group)
	    btk_window_group_add_window (BTK_WINDOW (toplevel)->group, 
					 BTK_WINDOW (grab_widget));
	}

      priv->dropper_grab_widget = grab_widget;
    }

  if (bdk_keyboard_grab (priv->dropper_grab_widget->window,
                         FALSE, time) != BDK_GRAB_SUCCESS)
    return;
  
  picker_cursor = make_picker_cursor (screen);
  grab_status = bdk_pointer_grab (priv->dropper_grab_widget->window,
				  FALSE,
				  BDK_BUTTON_RELEASE_MASK | BDK_BUTTON_PRESS_MASK | BDK_POINTER_MOTION_MASK,
				  NULL,
				  picker_cursor,
				  time);
  bdk_cursor_unref (picker_cursor);
  
  if (grab_status != BDK_GRAB_SUCCESS)
    {
      bdk_display_keyboard_ungrab (btk_widget_get_display (button), time);
      return;
    }

  btk_grab_add (priv->dropper_grab_widget);
  priv->grab_time = time;
  priv->has_grab = TRUE;
  
  g_signal_connect (priv->dropper_grab_widget, "button-press-event",
                    G_CALLBACK (mouse_press), colorsel);
  g_signal_connect (priv->dropper_grab_widget, "key-press-event",
                    G_CALLBACK (key_press), colorsel);
}

static void
hex_changed (BtkWidget *hex_entry,
	     gpointer   data)
{
  BtkColorSelection *colorsel;
  ColorSelectionPrivate *priv;
  BdkColor color;
  gchar *text;
  
  colorsel = BTK_COLOR_SELECTION (data);
  priv = colorsel->private_data;
  
  if (priv->changing)
    return;
  
  text = btk_editable_get_chars (BTK_EDITABLE (priv->hex_entry), 0, -1);
  if (bdk_color_parse (text, &color))
    {
      priv->color[COLORSEL_RED] = CLAMP (color.red/65535.0, 0.0, 1.0);
      priv->color[COLORSEL_GREEN] = CLAMP (color.green/65535.0, 0.0, 1.0);
      priv->color[COLORSEL_BLUE] = CLAMP (color.blue/65535.0, 0.0, 1.0);
      btk_rgb_to_hsv (priv->color[COLORSEL_RED],
		      priv->color[COLORSEL_GREEN],
		      priv->color[COLORSEL_BLUE],
		      &priv->color[COLORSEL_HUE],
		      &priv->color[COLORSEL_SATURATION],
		      &priv->color[COLORSEL_VALUE]);
      update_color (colorsel);
    }
  g_free (text);
}

static gboolean
hex_focus_out (BtkWidget     *hex_entry, 
	       BdkEventFocus *event,
	       gpointer       data)
{
  hex_changed (hex_entry, data);
  
  return FALSE;
}

static void
hsv_changed (BtkWidget *hsv,
	     gpointer   data)
{
  BtkColorSelection *colorsel;
  ColorSelectionPrivate *priv;
  
  colorsel = BTK_COLOR_SELECTION (data);
  priv = colorsel->private_data;
  
  if (priv->changing)
    return;
  
  btk_hsv_get_color (BTK_HSV (hsv),
		     &priv->color[COLORSEL_HUE],
		     &priv->color[COLORSEL_SATURATION],
		     &priv->color[COLORSEL_VALUE]);
  btk_hsv_to_rgb (priv->color[COLORSEL_HUE],
		  priv->color[COLORSEL_SATURATION],
		  priv->color[COLORSEL_VALUE],
		  &priv->color[COLORSEL_RED],
		  &priv->color[COLORSEL_GREEN],
		  &priv->color[COLORSEL_BLUE]);
  update_color (colorsel);
}

static void
adjustment_changed (BtkAdjustment *adjustment,
		    gpointer       data)
{
  BtkColorSelection *colorsel;
  ColorSelectionPrivate *priv;
  
  colorsel = BTK_COLOR_SELECTION (g_object_get_data (G_OBJECT (adjustment), "COLORSEL"));
  priv = colorsel->private_data;
  
  if (priv->changing)
    return;
  
  switch (GPOINTER_TO_INT (data))
    {
    case COLORSEL_SATURATION:
    case COLORSEL_VALUE:
      priv->color[GPOINTER_TO_INT (data)] = adjustment->value / 100;
      btk_hsv_to_rgb (priv->color[COLORSEL_HUE],
		      priv->color[COLORSEL_SATURATION],
		      priv->color[COLORSEL_VALUE],
		      &priv->color[COLORSEL_RED],
		      &priv->color[COLORSEL_GREEN],
		      &priv->color[COLORSEL_BLUE]);
      break;
    case COLORSEL_HUE:
      priv->color[GPOINTER_TO_INT (data)] = adjustment->value / 360;
      btk_hsv_to_rgb (priv->color[COLORSEL_HUE],
		      priv->color[COLORSEL_SATURATION],
		      priv->color[COLORSEL_VALUE],
		      &priv->color[COLORSEL_RED],
		      &priv->color[COLORSEL_GREEN],
		      &priv->color[COLORSEL_BLUE]);
      break;
    case COLORSEL_RED:
    case COLORSEL_GREEN:
    case COLORSEL_BLUE:
      priv->color[GPOINTER_TO_INT (data)] = adjustment->value / 255;
      
      btk_rgb_to_hsv (priv->color[COLORSEL_RED],
		      priv->color[COLORSEL_GREEN],
		      priv->color[COLORSEL_BLUE],
		      &priv->color[COLORSEL_HUE],
		      &priv->color[COLORSEL_SATURATION],
		      &priv->color[COLORSEL_VALUE]);
      break;
    default:
      priv->color[GPOINTER_TO_INT (data)] = adjustment->value / 255;
      break;
    }
  update_color (colorsel);
}

static void 
opacity_entry_changed (BtkWidget *opacity_entry,
		       gpointer   data)
{
  BtkColorSelection *colorsel;
  ColorSelectionPrivate *priv;
  BtkAdjustment *adj;
  gchar *text;
  
  colorsel = BTK_COLOR_SELECTION (data);
  priv = colorsel->private_data;
  
  if (priv->changing)
    return;
  
  text = btk_editable_get_chars (BTK_EDITABLE (priv->opacity_entry), 0, -1);
  adj = btk_range_get_adjustment (BTK_RANGE (priv->opacity_slider));
  btk_adjustment_set_value (adj, g_strtod (text, NULL)); 
  
  update_color (colorsel);
  
  g_free (text);
}

static void
make_label_spinbutton (BtkColorSelection *colorsel,
		       BtkWidget        **spinbutton,
		       gchar             *text,
		       BtkWidget         *table,
		       gint               i,
		       gint               j,
		       gint               channel_type,
                       const gchar       *tooltip)
{
  BtkWidget *label;
  BtkAdjustment *adjust;

  if (channel_type == COLORSEL_HUE)
    {
      adjust = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 360.0, 1.0, 1.0, 0.0));
    }
  else if (channel_type == COLORSEL_SATURATION ||
	   channel_type == COLORSEL_VALUE)
    {
      adjust = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 100.0, 1.0, 1.0, 0.0));
    }
  else
    {
      adjust = BTK_ADJUSTMENT (btk_adjustment_new (0.0, 0.0, 255.0, 1.0, 1.0, 0.0));
    }
  g_object_set_data (G_OBJECT (adjust), I_("COLORSEL"), colorsel);
  *spinbutton = btk_spin_button_new (adjust, 10.0, 0);

  btk_widget_set_tooltip_text (*spinbutton, tooltip);  

  g_signal_connect (adjust, "value-changed",
                    G_CALLBACK (adjustment_changed),
                    GINT_TO_POINTER (channel_type));
  label = btk_label_new_with_mnemonic (text);
  btk_label_set_mnemonic_widget (BTK_LABEL (label), *spinbutton);

  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_table_attach_defaults (BTK_TABLE (table), label, i, i+1, j, j+1);
  btk_table_attach_defaults (BTK_TABLE (table), *spinbutton, i+1, i+2, j, j+1);
}

static void
make_palette_frame (BtkColorSelection *colorsel,
		    BtkWidget         *table,
		    gint               i,
		    gint               j)
{
  BtkWidget *frame;
  ColorSelectionPrivate *priv;
  
  priv = colorsel->private_data;
  frame = btk_frame_new (NULL);
  btk_frame_set_shadow_type (BTK_FRAME (frame), BTK_SHADOW_IN);
  priv->custom_palette[i][j] = palette_new (colorsel);
  btk_widget_set_size_request (priv->custom_palette[i][j], CUSTOM_PALETTE_ENTRY_WIDTH, CUSTOM_PALETTE_ENTRY_HEIGHT);
  btk_container_add (BTK_CONTAINER (frame), priv->custom_palette[i][j]);
  btk_table_attach_defaults (BTK_TABLE (table), frame, i, i+1, j, j+1);
}

/* Set the palette entry [x][y] to be the currently selected one. */
static void 
set_selected_palette (BtkColorSelection *colorsel, int x, int y)
{
  ColorSelectionPrivate *priv = colorsel->private_data; 

  btk_widget_grab_focus (priv->custom_palette[x][y]);
}

static double
scale_round (double val, double factor)
{
  val = floor (val * factor + 0.5);
  val = MAX (val, 0);
  val = MIN (val, factor);
  return val;
}

static void
update_color (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv = colorsel->private_data;
  gchar entryval[12];
  gchar opacity_text[32];
  gchar *ptr;
  
  priv->changing = TRUE;
  color_sample_update_samples (colorsel);
  
  btk_hsv_set_color (BTK_HSV (priv->triangle_colorsel),
		     priv->color[COLORSEL_HUE],
		     priv->color[COLORSEL_SATURATION],
		     priv->color[COLORSEL_VALUE]);
  btk_adjustment_set_value (btk_spin_button_get_adjustment
			    (BTK_SPIN_BUTTON (priv->hue_spinbutton)),
			    scale_round (priv->color[COLORSEL_HUE], 360));
  btk_adjustment_set_value (btk_spin_button_get_adjustment
			    (BTK_SPIN_BUTTON (priv->sat_spinbutton)),
			    scale_round (priv->color[COLORSEL_SATURATION], 100));
  btk_adjustment_set_value (btk_spin_button_get_adjustment
			    (BTK_SPIN_BUTTON (priv->val_spinbutton)),
			    scale_round (priv->color[COLORSEL_VALUE], 100));
  btk_adjustment_set_value (btk_spin_button_get_adjustment
			    (BTK_SPIN_BUTTON (priv->red_spinbutton)),
			    scale_round (priv->color[COLORSEL_RED], 255));
  btk_adjustment_set_value (btk_spin_button_get_adjustment
			    (BTK_SPIN_BUTTON (priv->green_spinbutton)),
			    scale_round (priv->color[COLORSEL_GREEN], 255));
  btk_adjustment_set_value (btk_spin_button_get_adjustment
			    (BTK_SPIN_BUTTON (priv->blue_spinbutton)),
			    scale_round (priv->color[COLORSEL_BLUE], 255));
  btk_adjustment_set_value (btk_range_get_adjustment
			    (BTK_RANGE (priv->opacity_slider)),
			    scale_round (priv->color[COLORSEL_OPACITY], 255));
  
  g_snprintf (opacity_text, 32, "%.0f", scale_round (priv->color[COLORSEL_OPACITY], 255));
  btk_entry_set_text (BTK_ENTRY (priv->opacity_entry), opacity_text);
  
  g_snprintf (entryval, 11, "#%2X%2X%2X",
	      (guint) (scale_round (priv->color[COLORSEL_RED], 255)),
	      (guint) (scale_round (priv->color[COLORSEL_GREEN], 255)),
	      (guint) (scale_round (priv->color[COLORSEL_BLUE], 255)));
  
  for (ptr = entryval; *ptr; ptr++)
    if (*ptr == ' ')
      *ptr = '0';
  btk_entry_set_text (BTK_ENTRY (priv->hex_entry), entryval);
  priv->changing = FALSE;

  g_object_ref (colorsel);
  
  g_signal_emit (colorsel, color_selection_signals[COLOR_CHANGED], 0);
  
  g_object_freeze_notify (G_OBJECT (colorsel));
  g_object_notify (G_OBJECT (colorsel), "current-color");
  g_object_notify (G_OBJECT (colorsel), "current-alpha");
  g_object_thaw_notify (G_OBJECT (colorsel));
  
  g_object_unref (colorsel);
}

static void
update_palette (BtkColorSelection *colorsel)
{
  BdkColor *current_colors;
  gint i, j;

  current_colors = get_current_colors (colorsel);
  
  for (i = 0; i < BTK_CUSTOM_PALETTE_HEIGHT; i++)
    {
      for (j = 0; j < BTK_CUSTOM_PALETTE_WIDTH; j++)
	{
          gint index;

          index = i * BTK_CUSTOM_PALETTE_WIDTH + j;
          
          btk_color_selection_set_palette_color (colorsel,
                                                 index,
                                                 &current_colors[index]);
	}
    }

  g_free (current_colors);
}

static void
palette_change_notify_instance (GObject    *object,
                                GParamSpec *pspec,
                                gpointer    data)
{
  update_palette (BTK_COLOR_SELECTION (data));
}

static void
default_noscreen_change_palette_func (const BdkColor *colors,
				      gint            n_colors)
{
  default_change_palette_func (bdk_screen_get_default (), colors, n_colors);
}

static void
default_change_palette_func (BdkScreen	    *screen,
			     const BdkColor *colors,
                             gint            n_colors)
{
  gchar *str;
  
  str = btk_color_selection_palette_to_string (colors, n_colors);

  btk_settings_set_string_property (btk_settings_get_for_screen (screen),
                                    "btk-color-palette",
                                    str,
                                    "btk_color_selection_palette_to_string");

  g_free (str);
}

/**
 * btk_color_selection_new:
 * 
 * Creates a new BtkColorSelection.
 * 
 * Return value: a new #BtkColorSelection
 **/
BtkWidget *
btk_color_selection_new (void)
{
  BtkColorSelection *colorsel;
  ColorSelectionPrivate *priv;
  gdouble color[4];
  color[0] = 1.0;
  color[1] = 1.0;
  color[2] = 1.0;
  color[3] = 1.0;
  
  colorsel = g_object_new (BTK_TYPE_COLOR_SELECTION, NULL);
  priv = colorsel->private_data;
  set_color_internal (colorsel, color);
  btk_color_selection_set_has_opacity_control (colorsel, TRUE);
  
  /* We want to make sure that default_set is FALSE */
  /* This way the user can still set it */
  priv->default_set = FALSE;
  priv->default_alpha_set = FALSE;
  
  return BTK_WIDGET (colorsel);
}


void
btk_color_selection_set_update_policy (BtkColorSelection *colorsel,
				       BtkUpdateType      policy)
{
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
}

/**
 * btk_color_selection_get_has_opacity_control:
 * @colorsel: a #BtkColorSelection.
 * 
 * Determines whether the colorsel has an opacity control.
 * 
 * Return value: %TRUE if the @colorsel has an opacity control.  %FALSE if it does't.
 **/
gboolean
btk_color_selection_get_has_opacity_control (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_COLOR_SELECTION (colorsel), FALSE);
  
  priv = colorsel->private_data;
  
  return priv->has_opacity;
}

/**
 * btk_color_selection_set_has_opacity_control:
 * @colorsel: a #BtkColorSelection.
 * @has_opacity: %TRUE if @colorsel can set the opacity, %FALSE otherwise.
 *
 * Sets the @colorsel to use or not use opacity.
 * 
 **/
void
btk_color_selection_set_has_opacity_control (BtkColorSelection *colorsel,
					     gboolean           has_opacity)
{
  ColorSelectionPrivate *priv;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  
  priv = colorsel->private_data;
  has_opacity = has_opacity != FALSE;
  
  if (priv->has_opacity != has_opacity)
    {
      priv->has_opacity = has_opacity;
      if (has_opacity)
	{
	  btk_widget_show (priv->opacity_slider);
	  btk_widget_show (priv->opacity_label);
	  btk_widget_show (priv->opacity_entry);
	}
      else
	{
	  btk_widget_hide (priv->opacity_slider);
	  btk_widget_hide (priv->opacity_label);
	  btk_widget_hide (priv->opacity_entry);
	}
      color_sample_update_samples (colorsel);
      
      g_object_notify (G_OBJECT (colorsel), "has-opacity-control");
    }
}

/**
 * btk_color_selection_get_has_palette:
 * @colorsel: a #BtkColorSelection.
 * 
 * Determines whether the color selector has a color palette.
 * 
 * Return value: %TRUE if the selector has a palette.  %FALSE if it hasn't.
 **/
gboolean
btk_color_selection_get_has_palette (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_COLOR_SELECTION (colorsel), FALSE);
  
  priv = colorsel->private_data;
  
  return priv->has_palette;
}

/**
 * btk_color_selection_set_has_palette:
 * @colorsel: a #BtkColorSelection.
 * @has_palette: %TRUE if palette is to be visible, %FALSE otherwise.
 *
 * Shows and hides the palette based upon the value of @has_palette.
 * 
 **/
void
btk_color_selection_set_has_palette (BtkColorSelection *colorsel,
				     gboolean           has_palette)
{
  ColorSelectionPrivate *priv;
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  
  priv = colorsel->private_data;
  has_palette = has_palette != FALSE;
  
  if (priv->has_palette != has_palette)
    {
      priv->has_palette = has_palette;
      if (has_palette)
	btk_widget_show (priv->palette_frame);
      else
	btk_widget_hide (priv->palette_frame);

      update_tooltips (colorsel);

      g_object_notify (G_OBJECT (colorsel), "has-palette");
    }
}

/**
 * btk_color_selection_set_current_color:
 * @colorsel: a #BtkColorSelection.
 * @color: A #BdkColor to set the current color with.
 *
 * Sets the current color to be @color.  The first time this is called, it will
 * also set the original color to be @color too.
 **/
void
btk_color_selection_set_current_color (BtkColorSelection *colorsel,
				       const BdkColor    *color)
{
  ColorSelectionPrivate *priv;
  gint i;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);

  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->color[COLORSEL_RED] = SCALE (color->red);
  priv->color[COLORSEL_GREEN] = SCALE (color->green);
  priv->color[COLORSEL_BLUE] = SCALE (color->blue);
  btk_rgb_to_hsv (priv->color[COLORSEL_RED],
		  priv->color[COLORSEL_GREEN],
		  priv->color[COLORSEL_BLUE],
		  &priv->color[COLORSEL_HUE],
		  &priv->color[COLORSEL_SATURATION],
		  &priv->color[COLORSEL_VALUE]);
  if (priv->default_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
	priv->old_color[i] = priv->color[i];
    }
  priv->default_set = TRUE;
  update_color (colorsel);
}

/**
 * btk_color_selection_set_current_alpha:
 * @colorsel: a #BtkColorSelection.
 * @alpha: an integer between 0 and 65535.
 *
 * Sets the current opacity to be @alpha.  The first time this is called, it will
 * also set the original opacity to be @alpha too.
 **/
void
btk_color_selection_set_current_alpha (BtkColorSelection *colorsel,
				       guint16            alpha)
{
  ColorSelectionPrivate *priv;
  gint i;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  
  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->color[COLORSEL_OPACITY] = SCALE (alpha);
  if (priv->default_alpha_set == FALSE)
    {
      for (i = 0; i < COLORSEL_NUM_CHANNELS; i++)
	priv->old_color[i] = priv->color[i];
    }
  priv->default_alpha_set = TRUE;
  update_color (colorsel);
}

/**
 * btk_color_selection_set_color:
 * @colorsel: a #BtkColorSelection.
 * @color: an array of 4 doubles specifying the red, green, blue and opacity 
 *   to set the current color to.
 *
 * Sets the current color to be @color.  The first time this is called, it will
 * also set the original color to be @color too.
 *
 * Deprecated: 2.0: Use btk_color_selection_set_current_color() instead.
 **/
void
btk_color_selection_set_color (BtkColorSelection    *colorsel,
			       gdouble              *color)
{
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));

  set_color_internal (colorsel, color);
}

/**
 * btk_color_selection_get_current_color:
 * @colorsel: a #BtkColorSelection.
 * @color: (out): a #BdkColor to fill in with the current color.
 *
 * Sets @color to be the current color in the BtkColorSelection widget.
 **/
void
btk_color_selection_get_current_color (BtkColorSelection *colorsel,
				       BdkColor          *color)
{
  ColorSelectionPrivate *priv;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);
  
  priv = colorsel->private_data;
  color->red = UNSCALE (priv->color[COLORSEL_RED]);
  color->green = UNSCALE (priv->color[COLORSEL_GREEN]);
  color->blue = UNSCALE (priv->color[COLORSEL_BLUE]);
}

/**
 * btk_color_selection_get_current_alpha:
 * @colorsel: a #BtkColorSelection.
 *
 * Returns the current alpha value.
 *
 * Return value: an integer between 0 and 65535.
 **/
guint16
btk_color_selection_get_current_alpha (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_COLOR_SELECTION (colorsel), 0);
  
  priv = colorsel->private_data;
  return priv->has_opacity ? UNSCALE (priv->color[COLORSEL_OPACITY]) : 65535;
}

/**
 * btk_color_selection_get_color:
 * @colorsel: a #BtkColorSelection.
 * @color: an array of 4 #gdouble to fill in with the current color.
 *
 * Sets @color to be the current color in the BtkColorSelection widget.
 *
 * Deprecated: 2.0: Use btk_color_selection_get_current_color() instead.
 **/
void
btk_color_selection_get_color (BtkColorSelection *colorsel,
			       gdouble           *color)
{
  ColorSelectionPrivate *priv;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  
  priv = colorsel->private_data;
  color[0] = priv->color[COLORSEL_RED];
  color[1] = priv->color[COLORSEL_GREEN];
  color[2] = priv->color[COLORSEL_BLUE];
  color[3] = priv->has_opacity ? priv->color[COLORSEL_OPACITY] : 65535;
}

/**
 * btk_color_selection_set_previous_color:
 * @colorsel: a #BtkColorSelection.
 * @color: a #BdkColor to set the previous color with.
 *
 * Sets the 'previous' color to be @color.  This function should be called with
 * some hesitations, as it might seem confusing to have that color change.
 * Calling btk_color_selection_set_current_color() will also set this color the first
 * time it is called.
 **/
void
btk_color_selection_set_previous_color (BtkColorSelection *colorsel,
					const BdkColor    *color)
{
  ColorSelectionPrivate *priv;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);
  
  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->old_color[COLORSEL_RED] = SCALE (color->red);
  priv->old_color[COLORSEL_GREEN] = SCALE (color->green);
  priv->old_color[COLORSEL_BLUE] = SCALE (color->blue);
  btk_rgb_to_hsv (priv->old_color[COLORSEL_RED],
		  priv->old_color[COLORSEL_GREEN],
		  priv->old_color[COLORSEL_BLUE],
		  &priv->old_color[COLORSEL_HUE],
		  &priv->old_color[COLORSEL_SATURATION],
		  &priv->old_color[COLORSEL_VALUE]);
  color_sample_update_samples (colorsel);
  priv->default_set = TRUE;
  priv->changing = FALSE;
}

/**
 * btk_color_selection_set_previous_alpha:
 * @colorsel: a #BtkColorSelection.
 * @alpha: an integer between 0 and 65535.
 *
 * Sets the 'previous' alpha to be @alpha.  This function should be called with
 * some hesitations, as it might seem confusing to have that alpha change.
 **/
void
btk_color_selection_set_previous_alpha (BtkColorSelection *colorsel,
					guint16            alpha)
{
  ColorSelectionPrivate *priv;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  
  priv = colorsel->private_data;
  priv->changing = TRUE;
  priv->old_color[COLORSEL_OPACITY] = SCALE (alpha);
  color_sample_update_samples (colorsel);
  priv->default_alpha_set = TRUE;
  priv->changing = FALSE;
}


/**
 * btk_color_selection_get_previous_color:
 * @colorsel: a #BtkColorSelection.
 * @color: (out): a #BdkColor to fill in with the original color value.
 *
 * Fills @color in with the original color value.
 **/
void
btk_color_selection_get_previous_color (BtkColorSelection *colorsel,
					BdkColor           *color)
{
  ColorSelectionPrivate *priv;
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (color != NULL);
  
  priv = colorsel->private_data;
  color->red = UNSCALE (priv->old_color[COLORSEL_RED]);
  color->green = UNSCALE (priv->old_color[COLORSEL_GREEN]);
  color->blue = UNSCALE (priv->old_color[COLORSEL_BLUE]);
}

/**
 * btk_color_selection_get_previous_alpha:
 * @colorsel: a #BtkColorSelection.
 *
 * Returns the previous alpha value.
 *
 * Return value: an integer between 0 and 65535.
 **/
guint16
btk_color_selection_get_previous_alpha (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_COLOR_SELECTION (colorsel), 0);
  
  priv = colorsel->private_data;
  return priv->has_opacity ? UNSCALE (priv->old_color[COLORSEL_OPACITY]) : 65535;
}

/**
 * btk_color_selection_set_palette_color:
 * @colorsel: a #BtkColorSelection.
 * @index: the color index of the palette.
 * @color: A #BdkColor to set the palette with.
 *
 * Sets the palette located at @index to have @color as its color.
 * 
 **/
static void
btk_color_selection_set_palette_color (BtkColorSelection   *colorsel,
				       gint                 index,
				       BdkColor            *color)
{
  ColorSelectionPrivate *priv;
  gint x, y;
  gdouble col[3];
  
  g_return_if_fail (BTK_IS_COLOR_SELECTION (colorsel));
  g_return_if_fail (index >= 0  && index < BTK_CUSTOM_PALETTE_WIDTH*BTK_CUSTOM_PALETTE_HEIGHT);

  x = index % BTK_CUSTOM_PALETTE_WIDTH;
  y = index / BTK_CUSTOM_PALETTE_WIDTH;
  
  priv = colorsel->private_data;
  col[0] = SCALE (color->red);
  col[1] = SCALE (color->green);
  col[2] = SCALE (color->blue);
  
  palette_set_color (priv->custom_palette[x][y], colorsel, col);
}

/**
 * btk_color_selection_is_adjusting:
 * @colorsel: a #BtkColorSelection.
 *
 * Gets the current state of the @colorsel.
 *
 * Return value: %TRUE if the user is currently dragging a color around, and %FALSE
 * if the selection has stopped.
 **/
gboolean
btk_color_selection_is_adjusting (BtkColorSelection *colorsel)
{
  ColorSelectionPrivate *priv;
  
  g_return_val_if_fail (BTK_IS_COLOR_SELECTION (colorsel), FALSE);
  
  priv = colorsel->private_data;
  
  return (btk_hsv_is_adjusting (BTK_HSV (priv->triangle_colorsel)));
}


/**
 * btk_color_selection_palette_from_string:
 * @str: a string encoding a color palette.
 * @colors: (out) (array length=n_colors): return location for allocated
 *          array of #BdkColor.
 * @n_colors: return location for length of array.
 * 
 * Parses a color palette string; the string is a colon-separated
 * list of color names readable by bdk_color_parse().
 * 
 * Return value: %TRUE if a palette was successfully parsed.
 **/
gboolean
btk_color_selection_palette_from_string (const gchar *str,
                                         BdkColor   **colors,
                                         gint        *n_colors)
{
  BdkColor *retval;
  gint count;
  gchar *p;
  gchar *start;
  gchar *copy;
  
  count = 0;
  retval = NULL;
  copy = g_strdup (str);

  start = copy;
  p = copy;
  while (TRUE)
    {
      if (*p == ':' || *p == '\0')
        {
          gboolean done = TRUE;

          if (start == p)
            {
              goto failed; /* empty entry */
            }
              
          if (*p)
            {
              *p = '\0';
              done = FALSE;
            }

          retval = g_renew (BdkColor, retval, count + 1);
          if (!bdk_color_parse (start, retval + count))
            {
              goto failed;
            }

          ++count;

          if (done)
            break;
          else
            start = p + 1;
        }

      ++p;
    }

  g_free (copy);
  
  if (colors)
    *colors = retval;
  else
    g_free (retval);

  if (n_colors)
    *n_colors = count;

  return TRUE;
  
 failed:
  g_free (copy);
  g_free (retval);

  if (colors)
    *colors = NULL;
  if (n_colors)
    *n_colors = 0;

  return FALSE;
}

/**
 * btk_color_selection_palette_to_string:
 * @colors: (array length=n_colors): an array of colors.
 * @n_colors: length of the array.
 * 
 * Encodes a palette as a string, useful for persistent storage.
 * 
 * Return value: allocated string encoding the palette.
 **/
gchar*
btk_color_selection_palette_to_string (const BdkColor *colors,
                                       gint            n_colors)
{
  gint i;
  gchar **strs = NULL;
  gchar *retval;
  
  if (n_colors == 0)
    return g_strdup ("");

  strs = g_new0 (gchar*, n_colors + 1);

  i = 0;
  while (i < n_colors)
    {
      gchar *ptr;
      
      strs[i] =
        g_strdup_printf ("#%2X%2X%2X",
                         colors[i].red / 256,
                         colors[i].green / 256,
                         colors[i].blue / 256);

      for (ptr = strs[i]; *ptr; ptr++)
        if (*ptr == ' ')
          *ptr = '0';
      
      ++i;
    }

  retval = g_strjoinv (":", strs);

  g_strfreev (strs);

  return retval;
}

/**
 * btk_color_selection_set_change_palette_hook:
 * @func: a function to call when the custom palette needs saving.
 * 
 * Installs a global function to be called whenever the user tries to
 * modify the palette in a color selection. This function should save
 * the new palette contents, and update the BtkSettings property
 * "btk-color-palette" so all BtkColorSelection widgets will be modified.
 *
 * Return value: the previous change palette hook (that was replaced).
 *
 * Deprecated: 2.4: This function does not work in multihead environments.
 *     Use btk_color_selection_set_change_palette_with_screen_hook() instead. 
 * 
 **/
BtkColorSelectionChangePaletteFunc
btk_color_selection_set_change_palette_hook (BtkColorSelectionChangePaletteFunc func)
{
  BtkColorSelectionChangePaletteFunc old;

  old = noscreen_change_palette_hook;

  noscreen_change_palette_hook = func;

  return old;
}

/**
 * btk_color_selection_set_change_palette_with_screen_hook:
 * @func: a function to call when the custom palette needs saving.
 * 
 * Installs a global function to be called whenever the user tries to
 * modify the palette in a color selection. This function should save
 * the new palette contents, and update the BtkSettings property
 * "btk-color-palette" so all BtkColorSelection widgets will be modified.
 * 
 * Return value: the previous change palette hook (that was replaced).
 *
 * Since: 2.2
 **/
BtkColorSelectionChangePaletteWithScreenFunc
btk_color_selection_set_change_palette_with_screen_hook (BtkColorSelectionChangePaletteWithScreenFunc func)
{
  BtkColorSelectionChangePaletteWithScreenFunc old;

  old = change_palette_hook;

  change_palette_hook = func;

  return old;
}

static void
make_control_relations (BatkObject *batk_obj,
                        BtkWidget *widget)
{
  BatkObject *obj;

  obj = btk_widget_get_accessible (widget);
  batk_object_add_relationship (batk_obj, BATK_RELATION_CONTROLLED_BY, obj);
  batk_object_add_relationship (obj, BATK_RELATION_CONTROLLER_FOR, batk_obj);
}

static void
make_all_relations (BatkObject *batk_obj,
                    ColorSelectionPrivate *priv)
{
  make_control_relations (batk_obj, priv->hue_spinbutton);
  make_control_relations (batk_obj, priv->sat_spinbutton);
  make_control_relations (batk_obj, priv->val_spinbutton);
  make_control_relations (batk_obj, priv->red_spinbutton);
  make_control_relations (batk_obj, priv->green_spinbutton);
  make_control_relations (batk_obj, priv->blue_spinbutton);
}

#define __BTK_COLOR_SELECTION_C__
#include "btkaliasdef.c"

