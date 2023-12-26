/* BTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Massively updated for Bango by Owen Taylor, May 2000
 * BtkFontSelection widget for Btk+, by Damon Chaplin, May 1998.
 * Based on the BunnyFontSelector widget, by Elliot Lee, but major changes.
 * The BunnyFontSelector was derived from app/text_tool.c in the GIMP.
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

#include "config.h"
#include <stdlib.h>
#include <bunnylib/gprintf.h>
#include <string.h>

#include <batk/batk.h>

#include "bdk/bdk.h"
#include "bdk/bdkkeysyms.h"

#include "btkfontsel.h"

#include "btkbutton.h"
#include "btkcellrenderertext.h"
#include "btkentry.h"
#include "btkframe.h"
#include "btkhbbox.h"
#include "btkhbox.h"
#include "btklabel.h"
#include "btkliststore.h"
#include "btkrc.h"
#include "btkstock.h"
#include "btktable.h"
#include "btktreeselection.h"
#include "btktreeview.h"
#include "btkvbox.h"
#include "btkscrolledwindow.h"
#include "btkintl.h"
#include "btkaccessible.h"
#include "btkprivate.h"
#include "btkbuildable.h"
#include "btkalias.h"

/* We don't enable the font and style entries because they don't add
 * much in terms of visible effect and have a weird effect on keynav.
 * the Windows font selector has entries similarly positioned but they
 * act in conjunction with the associated lists to form a single focus
 * location.
 */
#undef INCLUDE_FONT_ENTRIES

/* This is the default text shown in the preview entry, though the user
   can set it. Remember that some fonts only have capital letters. */
#define PREVIEW_TEXT N_("abcdefghijk ABCDEFGHIJK")

#define DEFAULT_FONT_NAME "Sans 10"

/* This is the initial and maximum height of the preview entry (it expands
   when large font sizes are selected). Initial height is also the minimum. */
#define INITIAL_PREVIEW_HEIGHT 44
#define MAX_PREVIEW_HEIGHT 300

/* These are the sizes of the font, style & size lists. */
#define FONT_LIST_HEIGHT	136
#define FONT_LIST_WIDTH		190
#define FONT_STYLE_LIST_WIDTH	170
#define FONT_SIZE_LIST_WIDTH	60

/* These are what we use as the standard font sizes, for the size list.
 */
static const guint16 font_sizes[] = {
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 22, 24, 26, 28,
  32, 36, 40, 48, 56, 64, 72
};

enum {
   PROP_0,
   PROP_FONT_NAME,
   PROP_FONT,
   PROP_PREVIEW_TEXT
};


enum {
  FAMILY_COLUMN,
  FAMILY_NAME_COLUMN
};

enum {
  FACE_COLUMN,
  FACE_NAME_COLUMN
};

enum {
  SIZE_COLUMN
};

static void    btk_font_selection_set_property       (GObject         *object,
						      guint            prop_id,
						      const GValue    *value,
						      GParamSpec      *pspec);
static void    btk_font_selection_get_property       (GObject         *object,
						      guint            prop_id,
						      GValue          *value,
						      GParamSpec      *pspec);
static void    btk_font_selection_finalize	     (GObject         *object);
static void    btk_font_selection_screen_changed     (BtkWidget	      *widget,
						      BdkScreen       *previous_screen);
static void    btk_font_selection_style_set          (BtkWidget      *widget,
						      BtkStyle       *prev_style);

/* These are the callbacks & related functions. */
static void     btk_font_selection_select_font           (BtkTreeSelection *selection,
							  gpointer          data);
static void     btk_font_selection_show_available_fonts  (BtkFontSelection *fs);

static void     btk_font_selection_show_available_styles (BtkFontSelection *fs);
static void     btk_font_selection_select_best_style     (BtkFontSelection *fs,
							  gboolean          use_first);
static void     btk_font_selection_select_style          (BtkTreeSelection *selection,
							  gpointer          data);

static void     btk_font_selection_select_best_size      (BtkFontSelection *fs);
static void     btk_font_selection_show_available_sizes  (BtkFontSelection *fs,
							  gboolean          first_time);
static void     btk_font_selection_size_activate         (BtkWidget        *w,
							  gpointer          data);
static gboolean btk_font_selection_size_focus_out        (BtkWidget        *w,
							  BdkEventFocus    *event,
							  gpointer          data);
static void     btk_font_selection_select_size           (BtkTreeSelection *selection,
							  gpointer          data);

static void     btk_font_selection_scroll_on_map         (BtkWidget        *w,
							  gpointer          data);

static void     btk_font_selection_preview_changed       (BtkWidget        *entry,
							  BtkFontSelection *fontsel);
static void     btk_font_selection_scroll_to_selection   (BtkFontSelection *fontsel);


/* Misc. utility functions. */
static void    btk_font_selection_load_font          (BtkFontSelection *fs);
static void    btk_font_selection_update_preview     (BtkFontSelection *fs);

static BdkFont* btk_font_selection_get_font_internal (BtkFontSelection *fontsel);
static BangoFontDescription *btk_font_selection_get_font_description (BtkFontSelection *fontsel);
static gboolean btk_font_selection_select_font_desc  (BtkFontSelection      *fontsel,
						      BangoFontDescription  *new_desc,
						      BangoFontFamily      **pfamily,
						      BangoFontFace        **pface);
static void     btk_font_selection_reload_fonts          (BtkFontSelection *fontsel);
static void     btk_font_selection_ref_family            (BtkFontSelection *fontsel,
							  BangoFontFamily  *family);
static void     btk_font_selection_ref_face              (BtkFontSelection *fontsel,
							  BangoFontFace    *face);

G_DEFINE_TYPE (BtkFontSelection, btk_font_selection, BTK_TYPE_VBOX)

static void
btk_font_selection_class_init (BtkFontSelectionClass *klass)
{
  GObjectClass *bobject_class = G_OBJECT_CLASS (klass);
  BtkWidgetClass *widget_class = BTK_WIDGET_CLASS (klass);
  
  bobject_class->set_property = btk_font_selection_set_property;
  bobject_class->get_property = btk_font_selection_get_property;

  widget_class->screen_changed = btk_font_selection_screen_changed;
  widget_class->style_set = btk_font_selection_style_set;
   
  g_object_class_install_property (bobject_class,
                                   PROP_FONT_NAME,
                                   g_param_spec_string ("font-name",
                                                        P_("Font name"),
                                                        P_("The string that represents this font"),
                                                        DEFAULT_FONT_NAME,
                                                        BTK_PARAM_READWRITE));
  g_object_class_install_property (bobject_class,
				   PROP_FONT,
				   g_param_spec_boxed ("font",
						       P_("Font"),
						       P_("The BdkFont that is currently selected"),
						       BDK_TYPE_FONT,
						       BTK_PARAM_READABLE));
  g_object_class_install_property (bobject_class,
                                   PROP_PREVIEW_TEXT,
                                   g_param_spec_string ("preview-text",
                                                        P_("Preview text"),
                                                        P_("The text to display in order to demonstrate the selected font"),
                                                        _(PREVIEW_TEXT),
                                                        BTK_PARAM_READWRITE));
  bobject_class->finalize = btk_font_selection_finalize;
}

static void 
btk_font_selection_set_property (GObject         *object,
				 guint            prop_id,
				 const GValue    *value,
				 GParamSpec      *pspec)
{
  BtkFontSelection *fontsel;

  fontsel = BTK_FONT_SELECTION (object);

  switch (prop_id)
    {
    case PROP_FONT_NAME:
      btk_font_selection_set_font_name (fontsel, g_value_get_string (value));
      break;
    case PROP_PREVIEW_TEXT:
      btk_font_selection_set_preview_text (fontsel, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void btk_font_selection_get_property (GObject         *object,
					     guint            prop_id,
					     GValue          *value,
					     GParamSpec      *pspec)
{
  BtkFontSelection *fontsel;

  fontsel = BTK_FONT_SELECTION (object);

  switch (prop_id)
    {
    case PROP_FONT_NAME:
      g_value_take_string (value, btk_font_selection_get_font_name (fontsel));
      break;
    case PROP_FONT:
      g_value_set_boxed (value, btk_font_selection_get_font_internal (fontsel));
      break;
    case PROP_PREVIEW_TEXT:
      g_value_set_string (value, btk_font_selection_get_preview_text (fontsel));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* Handles key press events on the lists, so that we can trap Enter to
 * activate the default button on our own.
 */
static gboolean
list_row_activated (BtkWidget *widget)
{
  BtkWindow *window;
  
  window = BTK_WINDOW (btk_widget_get_toplevel (BTK_WIDGET (widget)));
  if (!btk_widget_is_toplevel (BTK_WIDGET (window)))
    window = NULL;
  
  if (window
      && widget != window->default_widget
      && !(widget == window->focus_widget &&
	   (!window->default_widget || !btk_widget_get_sensitive (window->default_widget))))
    {
      btk_window_activate_default (window);
    }
  
  return TRUE;
}

static void
btk_font_selection_init (BtkFontSelection *fontsel)
{
  BtkWidget *scrolled_win;
  BtkWidget *text_box;
  BtkWidget *table, *label;
  BtkWidget *font_label, *style_label;
  BtkWidget *vbox;
  BtkListStore *model;
  BtkTreeViewColumn *column;
  GList *focus_chain = NULL;
  BatkObject *batk_obj;

  btk_widget_push_composite_child ();

  btk_box_set_spacing (BTK_BOX (fontsel), 12);
  fontsel->size = 12 * BANGO_SCALE;
  
  /* Create the table of font, style & size. */
  table = btk_table_new (3, 3, FALSE);
  btk_widget_show (table);
  btk_table_set_row_spacings (BTK_TABLE (table), 6);
  btk_table_set_col_spacings (BTK_TABLE (table), 12);
  btk_box_pack_start (BTK_BOX (fontsel), table, TRUE, TRUE, 0);

#ifdef INCLUDE_FONT_ENTRIES
  fontsel->font_entry = btk_entry_new ();
  btk_editable_set_editable (BTK_EDITABLE (fontsel->font_entry), FALSE);
  btk_widget_set_size_request (fontsel->font_entry, 20, -1);
  btk_widget_show (fontsel->font_entry);
  btk_table_attach (BTK_TABLE (table), fontsel->font_entry, 0, 1, 1, 2,
		    BTK_FILL, 0, 0, 0);
  
  fontsel->font_style_entry = btk_entry_new ();
  btk_editable_set_editable (BTK_EDITABLE (fontsel->font_style_entry), FALSE);
  btk_widget_set_size_request (fontsel->font_style_entry, 20, -1);
  btk_widget_show (fontsel->font_style_entry);
  btk_table_attach (BTK_TABLE (table), fontsel->font_style_entry, 1, 2, 1, 2,
		    BTK_FILL, 0, 0, 0);
#endif /* INCLUDE_FONT_ENTRIES */
  
  fontsel->size_entry = btk_entry_new ();
  btk_widget_set_size_request (fontsel->size_entry, 20, -1);
  btk_widget_show (fontsel->size_entry);
  btk_table_attach (BTK_TABLE (table), fontsel->size_entry, 2, 3, 1, 2,
		    BTK_FILL, 0, 0, 0);
  g_signal_connect (fontsel->size_entry, "activate",
		    G_CALLBACK (btk_font_selection_size_activate),
		    fontsel);
  g_signal_connect_after (fontsel->size_entry, "focus-out-event",
			  G_CALLBACK (btk_font_selection_size_focus_out),
			  fontsel);
  
  font_label = btk_label_new_with_mnemonic (_("_Family:"));
  btk_misc_set_alignment (BTK_MISC (font_label), 0.0, 0.5);
  btk_widget_show (font_label);
  btk_table_attach (BTK_TABLE (table), font_label, 0, 1, 0, 1,
		    BTK_FILL, 0, 0, 0);  

  style_label = btk_label_new_with_mnemonic (_("_Style:"));
  btk_misc_set_alignment (BTK_MISC (style_label), 0.0, 0.5);
  btk_widget_show (style_label);
  btk_table_attach (BTK_TABLE (table), style_label, 1, 2, 0, 1,
		    BTK_FILL, 0, 0, 0);
  
  label = btk_label_new_with_mnemonic (_("Si_ze:"));
  btk_label_set_mnemonic_widget (BTK_LABEL (label),
                                 fontsel->size_entry);
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_table_attach (BTK_TABLE (table), label, 2, 3, 0, 1,
		    BTK_FILL, 0, 0, 0);
  
  
  /* Create the lists  */

  model = btk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FAMILY_COLUMN */
			      G_TYPE_STRING); /* FAMILY_NAME_COLUMN */
  fontsel->family_list = btk_tree_view_new_with_model (BTK_TREE_MODEL (model));
  g_object_unref (model);

  g_signal_connect (fontsel->family_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  column = btk_tree_view_column_new_with_attributes ("Family",
						     btk_cell_renderer_text_new (),
						     "text", FAMILY_NAME_COLUMN,
						     NULL);
  btk_tree_view_column_set_sizing (column, BTK_TREE_VIEW_COLUMN_AUTOSIZE);
  btk_tree_view_append_column (BTK_TREE_VIEW (fontsel->family_list), column);

  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (fontsel->family_list), FALSE);
  btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->family_list)),
			       BTK_SELECTION_BROWSE);
  
  btk_label_set_mnemonic_widget (BTK_LABEL (font_label), fontsel->family_list);

  scrolled_win = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_win), BTK_SHADOW_IN);
  btk_widget_set_size_request (scrolled_win,
			       FONT_LIST_WIDTH, FONT_LIST_HEIGHT);
  btk_container_add (BTK_CONTAINER (scrolled_win), fontsel->family_list);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_ALWAYS);
  btk_widget_show (fontsel->family_list);
  btk_widget_show (scrolled_win);

  btk_table_attach (BTK_TABLE (table), scrolled_win, 0, 1, 1, 3,
		    BTK_EXPAND | BTK_FILL,
		    BTK_EXPAND | BTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);
  
  model = btk_list_store_new (2,
			      G_TYPE_OBJECT,  /* FACE_COLUMN */
			      G_TYPE_STRING); /* FACE_NAME_COLUMN */
  fontsel->face_list = btk_tree_view_new_with_model (BTK_TREE_MODEL (model));
  g_object_unref (model);
  g_signal_connect (fontsel->face_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  btk_label_set_mnemonic_widget (BTK_LABEL (style_label), fontsel->face_list);

  column = btk_tree_view_column_new_with_attributes ("Face",
						     btk_cell_renderer_text_new (),
						     "text", FACE_NAME_COLUMN,
						     NULL);
  btk_tree_view_column_set_sizing (column, BTK_TREE_VIEW_COLUMN_AUTOSIZE);
  btk_tree_view_append_column (BTK_TREE_VIEW (fontsel->face_list), column);

  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (fontsel->face_list), FALSE);
  btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->face_list)),
			       BTK_SELECTION_BROWSE);
  
  scrolled_win = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_win), BTK_SHADOW_IN);
  btk_widget_set_size_request (scrolled_win,
			       FONT_STYLE_LIST_WIDTH, FONT_LIST_HEIGHT);
  btk_container_add (BTK_CONTAINER (scrolled_win), fontsel->face_list);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				  BTK_POLICY_AUTOMATIC, BTK_POLICY_ALWAYS);
  btk_widget_show (fontsel->face_list);
  btk_widget_show (scrolled_win);
  btk_table_attach (BTK_TABLE (table), scrolled_win, 1, 2, 1, 3,
		    BTK_EXPAND | BTK_FILL,
		    BTK_EXPAND | BTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);
  
  focus_chain = g_list_append (focus_chain, fontsel->size_entry);

  model = btk_list_store_new (1, G_TYPE_INT);
  fontsel->size_list = btk_tree_view_new_with_model (BTK_TREE_MODEL (model));
  g_object_unref (model);
  g_signal_connect (fontsel->size_list, "row-activated",
		    G_CALLBACK (list_row_activated), fontsel);

  column = btk_tree_view_column_new_with_attributes ("Size",
						     btk_cell_renderer_text_new (),
						     "text", SIZE_COLUMN,
						     NULL);
  btk_tree_view_column_set_sizing (column, BTK_TREE_VIEW_COLUMN_AUTOSIZE);
  btk_tree_view_append_column (BTK_TREE_VIEW (fontsel->size_list), column);

  btk_tree_view_set_headers_visible (BTK_TREE_VIEW (fontsel->size_list), FALSE);
  btk_tree_selection_set_mode (btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->size_list)),
			       BTK_SELECTION_BROWSE);
  
  scrolled_win = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (scrolled_win), BTK_SHADOW_IN);
  btk_container_add (BTK_CONTAINER (scrolled_win), fontsel->size_list);
  btk_widget_set_size_request (scrolled_win, -1, FONT_LIST_HEIGHT);
  btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (scrolled_win),
				  BTK_POLICY_NEVER, BTK_POLICY_ALWAYS);
  btk_widget_show (fontsel->size_list);
  btk_widget_show (scrolled_win);
  btk_table_attach (BTK_TABLE (table), scrolled_win, 2, 3, 2, 3,
		    BTK_FILL, BTK_EXPAND | BTK_FILL, 0, 0);
  focus_chain = g_list_append (focus_chain, scrolled_win);

  btk_container_set_focus_chain (BTK_CONTAINER (table), focus_chain);
  g_list_free (focus_chain);
  
  /* Insert the fonts. */
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->family_list)), "changed",
		    G_CALLBACK (btk_font_selection_select_font), fontsel);

  g_signal_connect_after (fontsel->family_list, "map",
			  G_CALLBACK (btk_font_selection_scroll_on_map),
			  fontsel);
  
  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->face_list)), "changed",
		    G_CALLBACK (btk_font_selection_select_style), fontsel);

  g_signal_connect (btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->size_list)), "changed",
		    G_CALLBACK (btk_font_selection_select_size), fontsel);
  batk_obj = btk_widget_get_accessible (fontsel->size_list);
  if (BTK_IS_ACCESSIBLE (batk_obj))
    {
      /* Accessibility support is enabled.
       * Make the label BATK_RELATON_LABEL_FOR for the size list as well.
       */
      BatkObject *batk_label;
      BatkRelationSet *relation_set;
      BatkRelation *relation;
      BatkObject *obj_array[1];

      batk_label = btk_widget_get_accessible (label);
      relation_set = batk_object_ref_relation_set (batk_obj);
      relation = batk_relation_set_get_relation_by_type (relation_set, BATK_RELATION_LABELLED_BY);
      if (relation)
        {
          batk_relation_add_target (relation, batk_label);
        }
      else 
        {
          obj_array[0] = batk_label;
          relation = batk_relation_new (obj_array, 1, BATK_RELATION_LABELLED_BY);
          batk_relation_set_add (relation_set, relation);
        }
      g_object_unref (relation_set);

      relation_set = batk_object_ref_relation_set (batk_label);
      relation = batk_relation_set_get_relation_by_type (relation_set, BATK_RELATION_LABEL_FOR);
      if (relation)
        {
          batk_relation_add_target (relation, batk_obj);
        }
      else 
        {
          obj_array[0] = batk_obj;
          relation = batk_relation_new (obj_array, 1, BATK_RELATION_LABEL_FOR);
          batk_relation_set_add (relation_set, relation);
        }
      g_object_unref (relation_set);
    }    
      

  vbox = btk_vbox_new (FALSE, 6);
  btk_widget_show (vbox);
  btk_box_pack_start (BTK_BOX (fontsel), vbox, FALSE, TRUE, 0);
  
  /* create the text entry widget */
  label = btk_label_new_with_mnemonic (_("_Preview:"));
  btk_misc_set_alignment (BTK_MISC (label), 0.0, 0.5);
  btk_widget_show (label);
  btk_box_pack_start (BTK_BOX (vbox), label, FALSE, TRUE, 0);
  
  text_box = btk_hbox_new (FALSE, 0);
  btk_widget_show (text_box);
  btk_box_pack_start (BTK_BOX (vbox), text_box, FALSE, TRUE, 0);
  
  fontsel->preview_entry = btk_entry_new ();
  btk_label_set_mnemonic_widget (BTK_LABEL (label), fontsel->preview_entry);
  btk_entry_set_text (BTK_ENTRY (fontsel->preview_entry), _(PREVIEW_TEXT));
  
  btk_widget_show (fontsel->preview_entry);
  g_signal_connect (fontsel->preview_entry, "changed",
		    G_CALLBACK (btk_font_selection_preview_changed), fontsel);
  btk_widget_set_size_request (fontsel->preview_entry,
			       -1, INITIAL_PREVIEW_HEIGHT);
  btk_box_pack_start (BTK_BOX (text_box), fontsel->preview_entry,
		      TRUE, TRUE, 0);
  btk_widget_pop_composite_child();
}

/**
 * btk_font_selection_new:
 *
 * Creates a new #BtkFontSelection.
 *
 * Return value: a n ew #BtkFontSelection
 */
BtkWidget *
btk_font_selection_new (void)
{
  BtkFontSelection *fontsel;
  
  fontsel = g_object_new (BTK_TYPE_FONT_SELECTION, NULL);
  
  return BTK_WIDGET (fontsel);
}

static void
btk_font_selection_finalize (GObject *object)
{
  BtkFontSelection *fontsel;
  
  g_return_if_fail (BTK_IS_FONT_SELECTION (object));
  
  fontsel = BTK_FONT_SELECTION (object);

  if (fontsel->font)
    bdk_font_unref (fontsel->font);

  btk_font_selection_ref_family (fontsel, NULL);
  btk_font_selection_ref_face (fontsel, NULL);

  G_OBJECT_CLASS (btk_font_selection_parent_class)->finalize (object);
}

static void
btk_font_selection_ref_family (BtkFontSelection *fontsel,
			       BangoFontFamily  *family)
{
  if (family)
    family = g_object_ref (family);
  if (fontsel->family)
    g_object_unref (fontsel->family);
  fontsel->family = family;
}

static void btk_font_selection_ref_face (BtkFontSelection *fontsel,
					 BangoFontFace    *face)
{
  if (face)
    face = g_object_ref (face);
  if (fontsel->face)
    g_object_unref (fontsel->face);
  fontsel->face = face;
}

static void
btk_font_selection_reload_fonts (BtkFontSelection *fontsel)
{
  if (btk_widget_has_screen (BTK_WIDGET (fontsel)))
    {
      BangoFontDescription *desc;
      desc = btk_font_selection_get_font_description (fontsel);

      btk_font_selection_show_available_fonts (fontsel);
      btk_font_selection_show_available_sizes (fontsel, TRUE);
      btk_font_selection_show_available_styles (fontsel);

      btk_font_selection_select_font_desc (fontsel, desc, NULL, NULL);
      btk_font_selection_scroll_to_selection (fontsel);

      bango_font_description_free (desc);
    }
}

static void
btk_font_selection_screen_changed (BtkWidget *widget,
				   BdkScreen *previous_screen)
{
  btk_font_selection_reload_fonts (BTK_FONT_SELECTION (widget));
}

static void
btk_font_selection_style_set (BtkWidget *widget,
			      BtkStyle  *prev_style)
{
  /* Maybe fonts where installed or removed... */
  btk_font_selection_reload_fonts (BTK_FONT_SELECTION (widget));
}

static void
btk_font_selection_preview_changed (BtkWidget        *entry,
				    BtkFontSelection *fontsel)
{
  g_object_notify (G_OBJECT (fontsel), "preview-text");
}

static void
scroll_to_selection (BtkTreeView *tree_view)
{
  BtkTreeSelection *selection = btk_tree_view_get_selection (tree_view);
  BtkTreeModel *model;
  BtkTreeIter iter;

  if (btk_tree_selection_get_selected (selection, &model, &iter))
    {
      BtkTreePath *path = btk_tree_model_get_path (model, &iter);
      btk_tree_view_scroll_to_cell (tree_view, path, NULL, TRUE, 0.5, 0.5);
      btk_tree_path_free (path);
    }
}

static void
set_cursor_to_iter (BtkTreeView *view,
		    BtkTreeIter *iter)
{
  BtkTreeModel *model = btk_tree_view_get_model (view);
  BtkTreePath *path = btk_tree_model_get_path (model, iter);
  
  btk_tree_view_set_cursor (view, path, NULL, FALSE);

  btk_tree_path_free (path);
}

static void
btk_font_selection_scroll_to_selection (BtkFontSelection *fontsel)
{
  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (BTK_TREE_VIEW (fontsel->family_list));
      
  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (BTK_TREE_VIEW (fontsel->face_list));
      
  /* Try to scroll the font family list to the selected item */
  scroll_to_selection (BTK_TREE_VIEW (fontsel->size_list));
/* This is called when the list is mapped. Here we scroll to the current
   font if necessary. */
}

static void
btk_font_selection_scroll_on_map (BtkWidget		*widget,
                                  gpointer		 data)
{
  btk_font_selection_scroll_to_selection (BTK_FONT_SELECTION (data));
}

/* This is called when a family is selected in the list. */
static void
btk_font_selection_select_font (BtkTreeSelection *selection,
				gpointer          data)
{
  BtkFontSelection *fontsel;
  BtkTreeModel *model;
  BtkTreeIter iter;
#ifdef INCLUDE_FONT_ENTRIES
  const gchar *family_name;
#endif

  fontsel = BTK_FONT_SELECTION (data);

  if (btk_tree_selection_get_selected (selection, &model, &iter))
    {
      BangoFontFamily *family;
      
      btk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      if (fontsel->family != family)
	{
	  btk_font_selection_ref_family (fontsel, family);
	  
#ifdef INCLUDE_FONT_ENTRIES
	  family_name = bango_font_family_get_name (fontsel->family);
	  btk_entry_set_text (BTK_ENTRY (fontsel->font_entry), family_name);
#endif
	  
	  btk_font_selection_show_available_styles (fontsel);
	  btk_font_selection_select_best_style (fontsel, TRUE);
	}

      g_object_unref (family);
    }
}

static int
cmp_families (const void *a, const void *b)
{
  const char *a_name = bango_font_family_get_name (*(BangoFontFamily **)a);
  const char *b_name = bango_font_family_get_name (*(BangoFontFamily **)b);
  
  return g_utf8_collate (a_name, b_name);
}

static void
btk_font_selection_show_available_fonts (BtkFontSelection *fontsel)
{
  BtkListStore *model;
  BangoFontFamily **families;
  BangoFontFamily *match_family = NULL;
  gint n_families, i;
  BtkTreeIter match_row;
  
  model = BTK_LIST_STORE (btk_tree_view_get_model (BTK_TREE_VIEW (fontsel->family_list)));
  
  bango_context_list_families (btk_widget_get_bango_context (BTK_WIDGET (fontsel)),
			       &families, &n_families);
  qsort (families, n_families, sizeof (BangoFontFamily *), cmp_families);

  btk_list_store_clear (model);

  for (i=0; i<n_families; i++)
    {
      const gchar *name = bango_font_family_get_name (families[i]);
      BtkTreeIter iter;

      btk_list_store_append (model, &iter);
      btk_list_store_set (model, &iter,
			  FAMILY_COLUMN, families[i],
			  FAMILY_NAME_COLUMN, name,
			  -1);
      
      if (i == 0 || !g_ascii_strcasecmp (name, "sans"))
	{
	  match_family = families[i];
	  match_row = iter;
	}
    }

  btk_font_selection_ref_family (fontsel, match_family);
  if (match_family)
    {
      set_cursor_to_iter (BTK_TREE_VIEW (fontsel->family_list), &match_row);
#ifdef INCLUDE_FONT_ENTRIES
      btk_entry_set_text (BTK_ENTRY (fontsel->font_entry), 
			  bango_font_family_get_name (match_family));
#endif /* INCLUDE_FONT_ENTRIES */
    }

  g_free (families);
}

static int
compare_font_descriptions (const BangoFontDescription *a, const BangoFontDescription *b)
{
  int val = strcmp (bango_font_description_get_family (a), bango_font_description_get_family (b));
  if (val != 0)
    return val;

  if (bango_font_description_get_weight (a) != bango_font_description_get_weight (b))
    return bango_font_description_get_weight (a) - bango_font_description_get_weight (b);

  if (bango_font_description_get_style (a) != bango_font_description_get_style (b))
    return bango_font_description_get_style (a) - bango_font_description_get_style (b);
  
  if (bango_font_description_get_stretch (a) != bango_font_description_get_stretch (b))
    return bango_font_description_get_stretch (a) - bango_font_description_get_stretch (b);

  if (bango_font_description_get_variant (a) != bango_font_description_get_variant (b))
    return bango_font_description_get_variant (a) - bango_font_description_get_variant (b);

  return 0;
}

static int
faces_sort_func (const void *a, const void *b)
{
  BangoFontDescription *desc_a = bango_font_face_describe (*(BangoFontFace **)a);
  BangoFontDescription *desc_b = bango_font_face_describe (*(BangoFontFace **)b);
  
  int ord = compare_font_descriptions (desc_a, desc_b);

  bango_font_description_free (desc_a);
  bango_font_description_free (desc_b);

  return ord;
}

static gboolean
font_description_style_equal (const BangoFontDescription *a,
			      const BangoFontDescription *b)
{
  return (bango_font_description_get_weight (a) == bango_font_description_get_weight (b) &&
	  bango_font_description_get_style (a) == bango_font_description_get_style (b) &&
	  bango_font_description_get_stretch (a) == bango_font_description_get_stretch (b) &&
	  bango_font_description_get_variant (a) == bango_font_description_get_variant (b));
}

/* This fills the font style list with all the possible style combinations
   for the current font family. */
static void
btk_font_selection_show_available_styles (BtkFontSelection *fontsel)
{
  gint n_faces, i;
  BangoFontFace **faces;
  BangoFontDescription *old_desc;
  BtkListStore *model;
  BtkTreeIter match_row;
  BangoFontFace *match_face = NULL;
  
  model = BTK_LIST_STORE (btk_tree_view_get_model (BTK_TREE_VIEW (fontsel->face_list)));
  
  if (fontsel->face)
    old_desc = bango_font_face_describe (fontsel->face);
  else
    old_desc= NULL;

  bango_font_family_list_faces (fontsel->family, &faces, &n_faces);
  qsort (faces, n_faces, sizeof (BangoFontFace *), faces_sort_func);

  btk_list_store_clear (model);

  for (i=0; i < n_faces; i++)
    {
      BtkTreeIter iter;
      const gchar *str = bango_font_face_get_face_name (faces[i]);

      btk_list_store_append (model, &iter);
      btk_list_store_set (model, &iter,
			  FACE_COLUMN, faces[i],
			  FACE_NAME_COLUMN, str,
			  -1);

      if (i == 0)
	{
	  match_row = iter;
	  match_face = faces[i];
	}
      else if (old_desc)
	{
	  BangoFontDescription *tmp_desc = bango_font_face_describe (faces[i]);
	  
	  if (font_description_style_equal (tmp_desc, old_desc))
	    {
	      match_row = iter;
	      match_face = faces[i];
	    }
      
	  bango_font_description_free (tmp_desc);
	}
    }

  if (old_desc)
    bango_font_description_free (old_desc);

  btk_font_selection_ref_face (fontsel, match_face);
  if (match_face)
    {
#ifdef INCLUDE_FONT_ENTRIES
      const gchar *str = bango_font_face_get_face_name (fontsel->face);

      btk_entry_set_text (BTK_ENTRY (fontsel->font_style_entry), str);
#endif      
      set_cursor_to_iter (BTK_TREE_VIEW (fontsel->face_list), &match_row);
    }

  g_free (faces);
}

/* This selects a style when the user selects a font. It just uses the first
   available style at present. I was thinking of trying to maintain the
   selected style, e.g. bold italic, when the user selects different fonts.
   However, the interface is so easy to use now I'm not sure it's worth it.
   Note: This will load a font. */
static void
btk_font_selection_select_best_style (BtkFontSelection *fontsel,
				      gboolean	        use_first)
{
  BtkTreeIter iter;
  BtkTreeModel *model;

  model = btk_tree_view_get_model (BTK_TREE_VIEW (fontsel->face_list));

  if (btk_tree_model_get_iter_first (model, &iter))
    {
      set_cursor_to_iter (BTK_TREE_VIEW (fontsel->face_list), &iter);
      scroll_to_selection (BTK_TREE_VIEW (fontsel->face_list));
    }

  btk_font_selection_show_available_sizes (fontsel, FALSE);
  btk_font_selection_select_best_size (fontsel);
}


/* This is called when a style is selected in the list. */
static void
btk_font_selection_select_style (BtkTreeSelection *selection,
				 gpointer          data)
{
  BtkFontSelection *fontsel = BTK_FONT_SELECTION (data);
  BtkTreeModel *model;
  BtkTreeIter iter;
  
  if (btk_tree_selection_get_selected (selection, &model, &iter))
    {
      BangoFontFace *face;
      
      btk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      btk_font_selection_ref_face (fontsel, face);
      g_object_unref (face);
    }

  btk_font_selection_show_available_sizes (fontsel, FALSE);
  btk_font_selection_select_best_size (fontsel);
}

static void
btk_font_selection_show_available_sizes (BtkFontSelection *fontsel,
					 gboolean          first_time)
{
  gint i;
  BtkListStore *model;
  gchar buffer[128];
  gchar *p;
      
  model = BTK_LIST_STORE (btk_tree_view_get_model (BTK_TREE_VIEW (fontsel->size_list)));

  /* Insert the standard font sizes */
  if (first_time)
    {
      btk_list_store_clear (model);

      for (i = 0; i < G_N_ELEMENTS (font_sizes); i++)
	{
	  BtkTreeIter iter;
	  
	  btk_list_store_append (model, &iter);
	  btk_list_store_set (model, &iter, SIZE_COLUMN, font_sizes[i], -1);
	  
	  if (font_sizes[i] * BANGO_SCALE == fontsel->size)
	    set_cursor_to_iter (BTK_TREE_VIEW (fontsel->size_list), &iter);
	}
    }
  else
    {
      BtkTreeIter iter;
      gboolean found = FALSE;
      
      btk_tree_model_get_iter_first (BTK_TREE_MODEL (model), &iter);
      for (i = 0; i < G_N_ELEMENTS (font_sizes) && !found; i++)
	{
	  if (font_sizes[i] * BANGO_SCALE == fontsel->size)
	    {
	      set_cursor_to_iter (BTK_TREE_VIEW (fontsel->size_list), &iter);
	      found = TRUE;
	    }

	  btk_tree_model_iter_next (BTK_TREE_MODEL (model), &iter);
	}

      if (!found)
	{
	  BtkTreeSelection *selection = btk_tree_view_get_selection (BTK_TREE_VIEW (fontsel->size_list));
	  btk_tree_selection_unselect_all (selection);
	}
    }

  /* Set the entry to the new size, rounding to 1 digit,
   * trimming of trailing 0's and a trailing period
   */
  g_snprintf (buffer, sizeof (buffer), "%.1f", fontsel->size / (1.0 * BANGO_SCALE));
  if (strchr (buffer, '.'))
    {
      p = buffer + strlen (buffer) - 1;
      while (*p == '0')
	p--;
      if (*p == '.')
	p--;
      p[1] = '\0';
    }

  /* Compare, to avoid moving the cursor unecessarily */
  if (strcmp (btk_entry_get_text (BTK_ENTRY (fontsel->size_entry)), buffer) != 0)
    btk_entry_set_text (BTK_ENTRY (fontsel->size_entry), buffer);
}

static void
btk_font_selection_select_best_size (BtkFontSelection *fontsel)
{
  btk_font_selection_load_font (fontsel);  
}

static void
btk_font_selection_set_size (BtkFontSelection *fontsel,
			     gint              new_size)
{
  if (fontsel->size != new_size)
    {
      fontsel->size = new_size;

      btk_font_selection_show_available_sizes (fontsel, FALSE);      
      btk_font_selection_load_font (fontsel);
    }
}

/* If the user hits return in the font size entry, we change to the new font
   size. */
static void
btk_font_selection_size_activate (BtkWidget   *w,
                                  gpointer     data)
{
  BtkFontSelection *fontsel;
  gint new_size;
  const gchar *text;
  
  fontsel = BTK_FONT_SELECTION (data);

  text = btk_entry_get_text (BTK_ENTRY (fontsel->size_entry));
  new_size = MAX (0.1, atof (text) * BANGO_SCALE + 0.5);

  if (fontsel->size != new_size)
    btk_font_selection_set_size (fontsel, new_size);
  else 
    list_row_activated (w);
}

static gboolean
btk_font_selection_size_focus_out (BtkWidget     *w,
				   BdkEventFocus *event,
				   gpointer       data)
{
  BtkFontSelection *fontsel;
  gint new_size;
  const gchar *text;
  
  fontsel = BTK_FONT_SELECTION (data);

  text = btk_entry_get_text (BTK_ENTRY (fontsel->size_entry));
  new_size = MAX (0.1, atof (text) * BANGO_SCALE + 0.5);

  btk_font_selection_set_size (fontsel, new_size);
  
  return TRUE;
}

/* This is called when a size is selected in the list. */
static void
btk_font_selection_select_size (BtkTreeSelection *selection,
				gpointer          data)
{
  BtkFontSelection *fontsel;
  BtkTreeModel *model;
  BtkTreeIter iter;
  gint new_size;
  
  fontsel = BTK_FONT_SELECTION (data);
  
  if (btk_tree_selection_get_selected (selection, &model, &iter))
    {
      btk_tree_model_get (model, &iter, SIZE_COLUMN, &new_size, -1);
      btk_font_selection_set_size (fontsel, new_size * BANGO_SCALE);
    }
}

static void
btk_font_selection_load_font (BtkFontSelection *fontsel)
{
  if (fontsel->font)
    bdk_font_unref (fontsel->font);
  fontsel->font = NULL;

  btk_font_selection_update_preview (fontsel);
}

static BangoFontDescription *
btk_font_selection_get_font_description (BtkFontSelection *fontsel)
{
  BangoFontDescription *font_desc;

  if (fontsel->face)
    {
      font_desc = bango_font_face_describe (fontsel->face);
      bango_font_description_set_size (font_desc, fontsel->size);
    }
  else
    font_desc = bango_font_description_from_string (DEFAULT_FONT_NAME);

  return font_desc;
}

/* This sets the font in the preview entry to the selected font, and tries to
   make sure that the preview entry is a reasonable size, i.e. so that the
   text can be seen with a bit of space to spare. But it tries to avoid
   resizing the entry every time the font changes.
   This also used to shrink the preview if the font size was decreased, but
   that made it awkward if the user wanted to resize the window themself. */
static void
btk_font_selection_update_preview (BtkFontSelection *fontsel)
{
  BtkRcStyle *rc_style;
  gint new_height;
  BtkRequisition old_requisition;
  BtkWidget *preview_entry = fontsel->preview_entry;
  const gchar *text;

  btk_widget_get_child_requisition (preview_entry, &old_requisition);
  
  rc_style = btk_rc_style_new ();
  rc_style->font_desc = btk_font_selection_get_font_description (fontsel);
  
  btk_widget_modify_style (preview_entry, rc_style);
  g_object_unref (rc_style);

  btk_widget_size_request (preview_entry, NULL);
  
  /* We don't ever want to be over MAX_PREVIEW_HEIGHT pixels high. */
  new_height = CLAMP (preview_entry->requisition.height, INITIAL_PREVIEW_HEIGHT, MAX_PREVIEW_HEIGHT);

  if (new_height > old_requisition.height || new_height < old_requisition.height - 30)
    btk_widget_set_size_request (preview_entry, -1, new_height);
  
  /* This sets the preview text, if it hasn't been set already. */
  text = btk_entry_get_text (BTK_ENTRY (preview_entry));
  if (strlen (text) == 0)
    btk_entry_set_text (BTK_ENTRY (preview_entry), _(PREVIEW_TEXT));
  btk_editable_set_position (BTK_EDITABLE (preview_entry), 0);
}

static BdkFont*
btk_font_selection_get_font_internal (BtkFontSelection *fontsel)
{
  if (!fontsel->font)
    {
      BangoFontDescription *font_desc = btk_font_selection_get_font_description (fontsel);
      fontsel->font = bdk_font_from_description_for_display (btk_widget_get_display (BTK_WIDGET (fontsel)), font_desc);
      bango_font_description_free (font_desc);
    }
  
  return fontsel->font;
}


/*****************************************************************************
 * These functions are the main public interface for getting/setting the font.
 *****************************************************************************/

/**
 * btk_font_selection_get_family_list:
 * @fontsel: a #BtkFontSelection
 *
 * This returns the #BtkTreeView that lists font families, for
 * example, 'Sans', 'Serif', etc.
 *
 * Return value: (transfer none): A #BtkWidget that is part of @fontsel
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_get_family_list (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->family_list;
}

/**
 * btk_font_selection_get_face_list:
 * @fontsel: a #BtkFontSelection
 *
 * This returns the #BtkTreeView which lists all styles available for
 * the selected font. For example, 'Regular', 'Bold', etc.
 * 
 * Return value: (transfer none): A #BtkWidget that is part of @fontsel
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_get_face_list (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->face_list;
}

/**
 * btk_font_selection_get_size_entry:
 * @fontsel: a #BtkFontSelection
 *
 * This returns the #BtkEntry used to allow the user to edit the font
 * number manually instead of selecting it from the list of font sizes.
 *
 * Return value: (transfer none): A #BtkWidget that is part of @fontsel
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_get_size_entry (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->size_entry;
}

/**
 * btk_font_selection_get_size_list:
 * @fontsel: a #BtkFontSelection
 *
 * This returns the #BtkTreeeView used to list font sizes.
 *
 * Return value: (transfer none): A #BtkWidget that is part of @fontsel
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_get_size_list (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->size_list;
}

/**
 * btk_font_selection_get_preview_entry:
 * @fontsel: a #BtkFontSelection
 *
 * This returns the #BtkEntry used to display the font as a preview.
 *
 * Return value: (transfer none): A #BtkWidget that is part of @fontsel
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_get_preview_entry (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->preview_entry;
}

/**
 * btk_font_selection_get_family:
 * @fontsel: a #BtkFontSelection
 *
 * Gets the #BangoFontFamily representing the selected font family.
 *
 * Return value: (transfer none): A #BangoFontFamily representing the
 *     selected font family. Font families are a collection of font
 *     faces. The returned object is owned by @fontsel and must not
 *     be modified or freed.
 *
 * Since: 2.14
 */
BangoFontFamily *
btk_font_selection_get_family (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->family;
}

/**
 * btk_font_selection_get_face:
 * @fontsel: a #BtkFontSelection
 *
 * Gets the #BangoFontFace representing the selected font group
 * details (i.e. family, slant, weight, width, etc).
 *
 * Return value: (transfer none): A #BangoFontFace representing the
 *     selected font group details. The returned object is owned by
 *     @fontsel and must not be modified or freed.
 *
 * Since: 2.14
 */
BangoFontFace *
btk_font_selection_get_face (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);
  
  return fontsel->face;
}

/**
 * btk_font_selection_get_size:
 * @fontsel: a #BtkFontSelection
 *
 * The selected font size.
 *
 * Return value: A n integer representing the selected font size,
 *     or -1 if no font size is selected.
 *
 * Since: 2.14
 **/
gint
btk_font_selection_get_size (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), -1);
  
  return fontsel->size;
}

/**
 * btk_font_selection_get_font:
 * @fontsel: a #BtkFontSelection
 *
 * Gets the currently-selected font.
 * 
 * Return value: A #BdkFont.
 *
 * Deprecated: 2.0: Use btk_font_selection_get_font_name() instead.
 */
BdkFont *
btk_font_selection_get_font (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);

  return btk_font_selection_get_font_internal (fontsel);
}

/**
 * btk_font_selection_get_font_name:
 * @fontsel: a #BtkFontSelection
 * 
 * Gets the currently-selected font name. 
 *
 * Note that this can be a different string than what you set with 
 * btk_font_selection_set_font_name(), as the font selection widget may 
 * normalize font names and thus return a string with a different structure. 
 * For example, "Helvetica Italic Bold 12" could be normalized to 
 * "Helvetica Bold Italic 12". Use bango_font_description_equal()
 * if you want to compare two font descriptions.
 * 
 * Return value: A string with the name of the current font, or %NULL if 
 *     no font is selected. You must free this string with g_free().
 */
gchar *
btk_font_selection_get_font_name (BtkFontSelection *fontsel)
{
  gchar *result;
  
  BangoFontDescription *font_desc = btk_font_selection_get_font_description (fontsel);
  result = bango_font_description_to_string (font_desc);
  bango_font_description_free (font_desc);

  return result;
}

/* This selects the appropriate list rows.
   First we check the fontname is valid and try to find the font family
   - i.e. the name in the main list. If we can't find that, then just return.
   Next we try to set each of the properties according to the fontname.
   Finally we select the font family & style in the lists. */
static gboolean
btk_font_selection_select_font_desc (BtkFontSelection      *fontsel,
				     BangoFontDescription  *new_desc,
				     BangoFontFamily      **pfamily,
				     BangoFontFace        **pface)
{
  BangoFontFamily *new_family = NULL;
  BangoFontFace *new_face = NULL;
  BangoFontFace *fallback_face = NULL;
  BtkTreeModel *model;
  BtkTreeIter iter;
  BtkTreeIter match_iter;
  gboolean valid;
  const gchar *new_family_name;

  new_family_name = bango_font_description_get_family (new_desc);

  if (!new_family_name)
    return FALSE;

  /* Check to make sure that this is in the list of allowed fonts 
   */
  model = btk_tree_view_get_model (BTK_TREE_VIEW (fontsel->family_list));
  for (valid = btk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = btk_tree_model_iter_next (model, &iter))
    {
      BangoFontFamily *family;
      
      btk_tree_model_get (model, &iter, FAMILY_COLUMN, &family, -1);
      
      if (g_ascii_strcasecmp (bango_font_family_get_name (family),
			      new_family_name) == 0)
	new_family = g_object_ref (family);

      g_object_unref (family);
      
      if (new_family)
	break;
    }

  if (!new_family)
    return FALSE;

  if (pfamily)
    *pfamily = new_family;
  else
    g_object_unref (new_family);
  set_cursor_to_iter (BTK_TREE_VIEW (fontsel->family_list), &iter);
  btk_font_selection_show_available_styles (fontsel);

  model = btk_tree_view_get_model (BTK_TREE_VIEW (fontsel->face_list));
  for (valid = btk_tree_model_get_iter_first (model, &iter);
       valid;
       valid = btk_tree_model_iter_next (model, &iter))
    {
      BangoFontFace *face;
      BangoFontDescription *tmp_desc;
      
      btk_tree_model_get (model, &iter, FACE_COLUMN, &face, -1);
      tmp_desc = bango_font_face_describe (face);
      
      if (font_description_style_equal (tmp_desc, new_desc))
	new_face = g_object_ref (face);
      
      if (!fallback_face)
	{
	  fallback_face = g_object_ref (face);
	  match_iter = iter;
	}
      
      bango_font_description_free (tmp_desc);
      g_object_unref (face);
      
      if (new_face)
	{
	  match_iter = iter;
	  break;
	}
    }

  if (!new_face)
    new_face = fallback_face;
  else if (fallback_face)
    g_object_unref (fallback_face);

  if (pface)
    *pface = new_face;
  else if (new_face)
    g_object_unref (new_face);
  set_cursor_to_iter (BTK_TREE_VIEW (fontsel->face_list), &match_iter);  

  btk_font_selection_set_size (fontsel, bango_font_description_get_size (new_desc));

  return TRUE;
}


/* This sets the current font, then selecting the appropriate list rows. */

/**
 * btk_font_selection_set_font_name:
 * @fontsel: a #BtkFontSelection
 * @fontname: a font name like "Helvetica 12" or "Times Bold 18"
 * 
 * Sets the currently-selected font. 
 *
 * Note that the @fontsel needs to know the screen in which it will appear 
 * for this to work; this can be guaranteed by simply making sure that the 
 * @fontsel is inserted in a toplevel window before you call this function.
 * 
 * Return value: %TRUE if the font could be set successfully; %FALSE if no 
 *     such font exists or if the @fontsel doesn't belong to a particular 
 *     screen yet.
 */
gboolean
btk_font_selection_set_font_name (BtkFontSelection *fontsel,
				  const gchar      *fontname)
{
  BangoFontFamily *family = NULL;
  BangoFontFace *face = NULL;
  BangoFontDescription *new_desc;
  
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), FALSE);

  if (!btk_widget_has_screen (BTK_WIDGET (fontsel)))
    return FALSE;

  new_desc = bango_font_description_from_string (fontname);

  if (btk_font_selection_select_font_desc (fontsel, new_desc, &family, &face))
    {
      btk_font_selection_ref_family (fontsel, family);
      if (family)
        g_object_unref (family);

      btk_font_selection_ref_face (fontsel, face);
      if (face)
        g_object_unref (face);
    }

  bango_font_description_free (new_desc);
  
  g_object_freeze_notify (G_OBJECT (fontsel));
  g_object_notify (G_OBJECT (fontsel), "font-name");
  g_object_notify (G_OBJECT (fontsel), "font");
  g_object_thaw_notify (G_OBJECT (fontsel));

  return TRUE;
}

/**
 * btk_font_selection_get_preview_text:
 * @fontsel: a #BtkFontSelection
 *
 * Gets the text displayed in the preview area.
 * 
 * Return value: the text displayed in the preview area. 
 *     This string is owned by the widget and should not be 
 *     modified or freed 
 */
const gchar*
btk_font_selection_get_preview_text (BtkFontSelection *fontsel)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION (fontsel), NULL);

  return btk_entry_get_text (BTK_ENTRY (fontsel->preview_entry));
}


/**
 * btk_font_selection_set_preview_text:
 * @fontsel: a #BtkFontSelection
 * @text: the text to display in the preview area 
 *
 * Sets the text displayed in the preview area.
 * The @text is used to show how the selected font looks.
 */
void
btk_font_selection_set_preview_text  (BtkFontSelection *fontsel,
				      const gchar      *text)
{
  g_return_if_fail (BTK_IS_FONT_SELECTION (fontsel));
  g_return_if_fail (text != NULL);

  btk_entry_set_text (BTK_ENTRY (fontsel->preview_entry), text);
}

/*****************************************************************************
 * BtkFontSelectionDialog
 *****************************************************************************/

static void btk_font_selection_dialog_buildable_interface_init     (BtkBuildableIface *iface);
static GObject * btk_font_selection_dialog_buildable_get_internal_child (BtkBuildable *buildable,
									  BtkBuilder   *builder,
									  const gchar  *childname);

G_DEFINE_TYPE_WITH_CODE (BtkFontSelectionDialog, btk_font_selection_dialog,
			 BTK_TYPE_DIALOG,
			 G_IMPLEMENT_INTERFACE (BTK_TYPE_BUILDABLE,
						btk_font_selection_dialog_buildable_interface_init))

static BtkBuildableIface *parent_buildable_iface;

static void
btk_font_selection_dialog_class_init (BtkFontSelectionDialogClass *klass)
{
}

static void
btk_font_selection_dialog_init (BtkFontSelectionDialog *fontseldiag)
{
  BtkDialog *dialog = BTK_DIALOG (fontseldiag);
  
  btk_dialog_set_has_separator (dialog, FALSE);
  btk_container_set_border_width (BTK_CONTAINER (dialog), 5);
  btk_box_set_spacing (BTK_BOX (dialog->vbox), 2); /* 2 * 5 + 2 = 12 */
  btk_container_set_border_width (BTK_CONTAINER (dialog->action_area), 5);
  btk_box_set_spacing (BTK_BOX (dialog->action_area), 6);

  btk_widget_push_composite_child ();

  btk_window_set_resizable (BTK_WINDOW (fontseldiag), TRUE);
  
  fontseldiag->main_vbox = dialog->vbox;
  
  fontseldiag->fontsel = btk_font_selection_new ();
  btk_container_set_border_width (BTK_CONTAINER (fontseldiag->fontsel), 5);
  btk_widget_show (fontseldiag->fontsel);
  btk_box_pack_start (BTK_BOX (fontseldiag->main_vbox),
		      fontseldiag->fontsel, TRUE, TRUE, 0);
  
  /* Create the action area */
  fontseldiag->action_area = dialog->action_area;

  fontseldiag->cancel_button = btk_dialog_add_button (dialog,
                                                      BTK_STOCK_CANCEL,
                                                      BTK_RESPONSE_CANCEL);

  fontseldiag->apply_button = btk_dialog_add_button (dialog,
                                                     BTK_STOCK_APPLY,
                                                     BTK_RESPONSE_APPLY);
  btk_widget_hide (fontseldiag->apply_button);

  fontseldiag->ok_button = btk_dialog_add_button (dialog,
                                                  BTK_STOCK_OK,
                                                  BTK_RESPONSE_OK);
  btk_widget_grab_default (fontseldiag->ok_button);
  
  btk_dialog_set_alternative_button_order (BTK_DIALOG (fontseldiag),
					   BTK_RESPONSE_OK,
					   BTK_RESPONSE_APPLY,
					   BTK_RESPONSE_CANCEL,
					   -1);

  btk_window_set_title (BTK_WINDOW (fontseldiag),
                        _("Font Selection"));

  btk_widget_pop_composite_child ();

  _btk_dialog_set_ignore_separator (dialog, TRUE);
}

/**
 * btk_font_selection_dialog_new:
 * @title: the title of the dialog window 
 *
 * Creates a new #BtkFontSelectionDialog.
 *
 * Return value: a new #BtkFontSelectionDialog
 */
BtkWidget*
btk_font_selection_dialog_new (const gchar *title)
{
  BtkFontSelectionDialog *fontseldiag;
  
  fontseldiag = g_object_new (BTK_TYPE_FONT_SELECTION_DIALOG, NULL);

  if (title)
    btk_window_set_title (BTK_WINDOW (fontseldiag), title);
  
  return BTK_WIDGET (fontseldiag);
}

/**
 * btk_font_selection_dialog_get_font_selection:
 * @fsd: a #BtkFontSelectionDialog
 *
 * Retrieves the #BtkFontSelection widget embedded in the dialog.
 *
 * Returns: (transfer none): the embedded #BtkFontSelection
 *
 * Since: 2.22
 **/
BtkWidget*
btk_font_selection_dialog_get_font_selection (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->fontsel;
}

/**
 * btk_font_selection_dialog_get_ok_button:
 * @fsd: a #BtkFontSelectionDialog
 *
 * Gets the 'OK' button.
 *
 * Return value: (transfer none): the #BtkWidget used in the dialog
 *     for the 'OK' button.
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_dialog_get_ok_button (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->ok_button;
}

/**
 * btk_font_selection_dialog_get_apply_button:
 * @fsd: a #BtkFontSelectionDialog
 *
 * Obtains a button. The button doesn't have any function.
 *
 * Return value: a #BtkWidget
 *
 * Since: 2.14
 *
 * Deprecated: 2.16: Don't use this function.
 */
BtkWidget *
btk_font_selection_dialog_get_apply_button (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->apply_button;
}

/**
 * btk_font_selection_dialog_get_cancel_button:
 * @fsd: a #BtkFontSelectionDialog
 *
 * Gets the 'Cancel' button.
 *
 * Return value: (transfer none): the #BtkWidget used in the dialog
 *     for the 'Cancel' button.
 *
 * Since: 2.14
 */
BtkWidget *
btk_font_selection_dialog_get_cancel_button (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return fsd->cancel_button;
}

static void
btk_font_selection_dialog_buildable_interface_init (BtkBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->get_internal_child = btk_font_selection_dialog_buildable_get_internal_child;
}

static GObject *
btk_font_selection_dialog_buildable_get_internal_child (BtkBuildable *buildable,
							BtkBuilder   *builder,
							const gchar  *childname)
{
    if (strcmp(childname, "ok_button") == 0)
	return G_OBJECT (BTK_FONT_SELECTION_DIALOG(buildable)->ok_button);
    else if (strcmp(childname, "cancel_button") == 0)
	return G_OBJECT (BTK_FONT_SELECTION_DIALOG (buildable)->cancel_button);
    else if (strcmp(childname, "apply_button") == 0)
	return G_OBJECT (BTK_FONT_SELECTION_DIALOG(buildable)->apply_button);
    else if (strcmp(childname, "font_selection") == 0)
	return G_OBJECT (BTK_FONT_SELECTION_DIALOG(buildable)->fontsel);

    return parent_buildable_iface->get_internal_child (buildable, builder, childname);
}

/**
 * btk_font_selection_dialog_get_font_name:
 * @fsd: a #BtkFontSelectionDialog
 * 
 * Gets the currently-selected font name.
 *
 * Note that this can be a different string than what you set with 
 * btk_font_selection_dialog_set_font_name(), as the font selection widget
 * may normalize font names and thus return a string with a different 
 * structure. For example, "Helvetica Italic Bold 12" could be normalized 
 * to "Helvetica Bold Italic 12".  Use bango_font_description_equal()
 * if you want to compare two font descriptions.
 * 
 * Return value: A string with the name of the current font, or %NULL if no 
 *     font is selected. You must free this string with g_free().
 */
gchar*
btk_font_selection_dialog_get_font_name (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return btk_font_selection_get_font_name (BTK_FONT_SELECTION (fsd->fontsel));
}

/**
 * btk_font_selection_dialog_get_font:
 * @fsd: a #BtkFontSelectionDialog
 *
 * Gets the currently-selected font.
 *
 * Return value: the #BdkFont from the #BtkFontSelection for the
 *     currently selected font in the dialog, or %NULL if no font is selected
 *
 * Deprecated: 2.0: Use btk_font_selection_dialog_get_font_name() instead.
 */
BdkFont*
btk_font_selection_dialog_get_font (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return btk_font_selection_get_font_internal (BTK_FONT_SELECTION (fsd->fontsel));
}

/**
 * btk_font_selection_dialog_set_font_name:
 * @fsd: a #BtkFontSelectionDialog
 * @fontname: a font name like "Helvetica 12" or "Times Bold 18"
 *
 * Sets the currently selected font. 
 * 
 * Return value: %TRUE if the font selected in @fsd is now the
 *     @fontname specified, %FALSE otherwise. 
 */
gboolean
btk_font_selection_dialog_set_font_name (BtkFontSelectionDialog *fsd,
					 const gchar	        *fontname)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), FALSE);
  g_return_val_if_fail (fontname, FALSE);

  return btk_font_selection_set_font_name (BTK_FONT_SELECTION (fsd->fontsel), fontname);
}

/**
 * btk_font_selection_dialog_get_preview_text:
 * @fsd: a #BtkFontSelectionDialog
 *
 * Gets the text displayed in the preview area.
 * 
 * Return value: the text displayed in the preview area. 
 *     This string is owned by the widget and should not be 
 *     modified or freed 
 */
const gchar*
btk_font_selection_dialog_get_preview_text (BtkFontSelectionDialog *fsd)
{
  g_return_val_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd), NULL);

  return btk_font_selection_get_preview_text (BTK_FONT_SELECTION (fsd->fontsel));
}

/**
 * btk_font_selection_dialog_set_preview_text:
 * @fsd: a #BtkFontSelectionDialog
 * @text: the text to display in the preview area
 *
 * Sets the text displayed in the preview area. 
 */
void
btk_font_selection_dialog_set_preview_text (BtkFontSelectionDialog *fsd,
					    const gchar	           *text)
{
  g_return_if_fail (BTK_IS_FONT_SELECTION_DIALOG (fsd));
  g_return_if_fail (text != NULL);

  btk_font_selection_set_preview_text (BTK_FONT_SELECTION (fsd->fontsel), text);
}

#define __BTK_FONTSEL_C__
#include "btkaliasdef.c"
