/* btktextmark.h - mark segments
 *
 * Copyright (c) 1994 The Regents of the University of California.
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

#ifndef __BTK_TEXT_MARK_H__
#define __BTK_TEXT_MARK_H__

#if defined(BTK_DISABLE_SINGLE_INCLUDES) && !defined (__BTK_H_INSIDE__) && !defined (BTK_COMPILATION)
#error "Only <btk/btk.h> can be included directly."
#endif

B_BEGIN_DECLS

/* The BtkTextMark data type */

typedef struct _BtkTextMark      BtkTextMark;
typedef struct _BtkTextMarkClass BtkTextMarkClass;

#define BTK_TYPE_TEXT_MARK              (btk_text_mark_get_type ())
#define BTK_TEXT_MARK(object)           (B_TYPE_CHECK_INSTANCE_CAST ((object), BTK_TYPE_TEXT_MARK, BtkTextMark))
#define BTK_TEXT_MARK_CLASS(klass)      (B_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_TEXT_MARK, BtkTextMarkClass))
#define BTK_IS_TEXT_MARK(object)        (B_TYPE_CHECK_INSTANCE_TYPE ((object), BTK_TYPE_TEXT_MARK))
#define BTK_IS_TEXT_MARK_CLASS(klass)   (B_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_TEXT_MARK))
#define BTK_TEXT_MARK_GET_CLASS(obj)    (B_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_TEXT_MARK, BtkTextMarkClass))

struct _BtkTextMark
{
  BObject parent_instance;

  bpointer GSEAL (segment);
};

struct _BtkTextMarkClass
{
  BObjectClass parent_class;

  /* Padding for future expansion */
  void (*_btk_reserved1) (void);
  void (*_btk_reserved2) (void);
  void (*_btk_reserved3) (void);
  void (*_btk_reserved4) (void);
};

GType        btk_text_mark_get_type   (void) B_GNUC_CONST;

void           btk_text_mark_set_visible (BtkTextMark *mark,
                                          bboolean     setting);
bboolean       btk_text_mark_get_visible (BtkTextMark *mark);

BtkTextMark          *btk_text_mark_new              (const bchar *name,
						      bboolean     left_gravity);
const bchar *         btk_text_mark_get_name         (BtkTextMark *mark);
bboolean              btk_text_mark_get_deleted      (BtkTextMark *mark);
BtkTextBuffer*        btk_text_mark_get_buffer       (BtkTextMark *mark);
bboolean              btk_text_mark_get_left_gravity (BtkTextMark *mark);

B_END_DECLS

#endif


