/* btktexttag.c - text tag object
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000      Red Hat, Inc.
 * Tk -> Btk port by Havoc Pennington <hp@redhat.com>
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */

#ifndef __BTK_TEXT_TAG_H__
#define __BTK_TEXT_TAG_H__


#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

#include <bdk/bdk.h>
#include <btk/btkenums.h>

/* Not needed, retained for compatibility -Yosh */
#include <btk/btkobject.h>


G_BEGIN_DECLS

typedef struct _BtkTextIter BtkTextIter;
typedef struct _BtkTextTagTable BtkTextTagTable;

typedef struct _BtkTextAttributes BtkTextAttributes;

#define BTK_TYPE_TEXT_TAG            (btk_text_tag_get_type ())
#define BTK_TEXT_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_TEXT_TAG, BtkTextTag))
#define BTK_TEXT_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_TAG, BtkTextTagClass))
#define BTK_IS_TEXT_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_TEXT_TAG))
#define BTK_IS_TEXT_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_TAG))
#define BTK_TEXT_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_TAG, BtkTextTagClass))

#define BTK_TYPE_TEXT_ATTRIBUTES     (btk_text_attributes_get_type ())

typedef struct _BtkTextTag BtkTextTag;
typedef struct _BtkTextTagClass BtkTextTagClass;

struct _BtkTextTag
{
  GObject parent_instance;

  BtkTextTagTable *GSEAL (table);

  char *GSEAL (name);           /* Name of this tag.  This field is actually
                                 * a pointer to the key from the entry in
                                 * tkxt->tagTable, so it needn't be freed
                                 * explicitly. */
  int GSEAL (priority);  /* Priority of this tag within widget.  0
                         * means lowest priority.  Exactly one tag
                         * has each integer value between 0 and
                         * numTags-1. */
  /*
   * Information for displaying text with this tag.  The information
   * belows acts as an override on information specified by lower-priority
   * tags.  If no value is specified, then the next-lower-priority tag
   * on the text determins the value.  The text widget itself provides
   * defaults if no tag specifies an override.
   */

  BtkTextAttributes *GSEAL (values);
  
  /* Flags for whether a given value is set; if a value is unset, then
   * this tag does not affect it.
   */
  guint GSEAL (bg_color_set) : 1;
  guint GSEAL (bg_stipple_set) : 1;
  guint GSEAL (fg_color_set) : 1;
  guint GSEAL (scale_set) : 1;
  guint GSEAL (fg_stipple_set) : 1;
  guint GSEAL (justification_set) : 1;
  guint GSEAL (left_margin_set) : 1;
  guint GSEAL (indent_set) : 1;
  guint GSEAL (rise_set) : 1;
  guint GSEAL (strikethrough_set) : 1;
  guint GSEAL (right_margin_set) : 1;
  guint GSEAL (pixels_above_lines_set) : 1;
  guint GSEAL (pixels_below_lines_set) : 1;
  guint GSEAL (pixels_inside_wrap_set) : 1;
  guint GSEAL (tabs_set) : 1;
  guint GSEAL (underline_set) : 1;
  guint GSEAL (wrap_mode_set) : 1;
  guint GSEAL (bg_full_height_set) : 1;
  guint GSEAL (invisible_set) : 1;
  guint GSEAL (editable_set) : 1;
  guint GSEAL (language_set) : 1;
  guint GSEAL (pg_bg_color_set) : 1;

  /* Whether these margins accumulate or override */
  guint GSEAL (accumulative_margin) : 1;

  guint GSEAL (pad1) : 1;
};

struct _BtkTextTagClass
{
  GObjectClass parent_class;

  gboolean (* event) (BtkTextTag        *tag,
                      GObject           *event_object, /* widget, canvas item, whatever */
                      BdkEvent          *event,        /* the event itself */
                      const BtkTextIter *iter);        /* location of event in buffer */

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType        btk_text_tag_get_type     (void) G_GNUC_CONST;
BtkTextTag  *btk_text_tag_new          (const gchar       *name);
gint         btk_text_tag_get_priority (BtkTextTag        *tag);
void         btk_text_tag_set_priority (BtkTextTag        *tag,
                                        gint               priority);
gboolean     btk_text_tag_event        (BtkTextTag        *tag,
                                        GObject           *event_object,
                                        BdkEvent          *event,
                                        const BtkTextIter *iter);

/*
 * Style object created by folding a set of tags together
 */

typedef struct _BtkTextAppearance BtkTextAppearance;

struct _BtkTextAppearance
{
  /*< public >*/
  BdkColor bg_color;
  BdkColor fg_color;
  BdkBitmap *bg_stipple;
  BdkBitmap *fg_stipple;

  /* super/subscript rise, can be negative */
  gint rise;

  /*< private >*/
  /* I'm not sure this can really be used without breaking some things
   * an app might do :-/
   */
  gpointer padding1;

  /*< public >*/
  guint underline : 4;          /* BangoUnderline */
  guint strikethrough : 1;

  /* Whether to use background-related values; this is irrelevant for
   * the values struct when in a tag, but is used for the composite
   * values struct; it's true if any of the tags being composited
   * had background stuff set.
   */
  guint draw_bg : 1;
  
  /* These are only used when we are actually laying out and rendering
   * a paragraph; not when a BtkTextAppearance is part of a
   * BtkTextAttributes.
   */
  guint inside_selection : 1;
  guint is_text : 1;

  /*< private >*/
  guint pad1 : 1;
  guint pad2 : 1;
  guint pad3 : 1;
  guint pad4 : 1;
};

struct _BtkTextAttributes
{
  /*< private >*/
  guint refcount;

  /*< public >*/
  BtkTextAppearance appearance;

  BtkJustification justification;
  BtkTextDirection direction;

  /* Individual chunks of this can be set/unset as a group */
  BangoFontDescription *font;

  gdouble font_scale;
  
  gint left_margin;

  gint indent;  

  gint right_margin;

  gint pixels_above_lines;

  gint pixels_below_lines;

  gint pixels_inside_wrap;

  BangoTabArray *tabs;

  BtkWrapMode wrap_mode;        /* How to handle wrap-around for this tag.
                                 * Must be BTK_WRAPMODE_CHAR,
                                 * BTK_WRAPMODE_NONE, BTK_WRAPMODE_WORD
                                 */

  BangoLanguage *language;

  /*< private >*/
  BdkColor *pg_bg_color;

  /*< public >*/
  /* hide the text  */
  guint invisible : 1;

  /* Background is fit to full line height rather than
   * baseline +/- ascent/descent (font height)
   */
  guint bg_full_height : 1;

  /* can edit this text */
  guint editable : 1;

  /* colors are allocated etc. */
  guint realized : 1;

  /*< private >*/
  guint pad1 : 1;
  guint pad2 : 1;
  guint pad3 : 1;
  guint pad4 : 1;
};

BtkTextAttributes* btk_text_attributes_new         (void);
BtkTextAttributes* btk_text_attributes_copy        (BtkTextAttributes *src);
void               btk_text_attributes_copy_values (BtkTextAttributes *src,
                                                    BtkTextAttributes *dest);
void               btk_text_attributes_unref       (BtkTextAttributes *values);
BtkTextAttributes *btk_text_attributes_ref         (BtkTextAttributes *values);

GType              btk_text_attributes_get_type    (void) G_GNUC_CONST;


G_END_DECLS

#endif

