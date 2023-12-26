/* BTK - The GIMP Toolkit
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
 * Author:  Theppitak Karoonboonyanan <thep@linux.thai.net>
 *
 */

#include <string.h>

#include <bdk/bdkkeysyms.h>
#include "btkimcontextthai.h"
#include "thai-charprop.h"

static void     btk_im_context_thai_class_init          (BtkIMContextThaiClass *class);
static void     btk_im_context_thai_init                (BtkIMContextThai      *im_context_thai);
static gboolean btk_im_context_thai_filter_keypress     (BtkIMContext          *context,
						         BdkEventKey           *key);

#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
static void     forget_previous_chars (BtkIMContextThai *context_thai);
static void     remember_previous_char (BtkIMContextThai *context_thai,
                                        gunichar new_char);
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */

static GObjectClass *parent_class;

GType btk_type_im_context_thai = 0;

void
btk_im_context_thai_register_type (GTypeModule *type_module)
{
  const GTypeInfo im_context_thai_info =
  {
    sizeof (BtkIMContextThaiClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) btk_im_context_thai_class_init,
    NULL,           /* class_finalize */    
    NULL,           /* class_data */
    sizeof (BtkIMContextThai),
    0,
    (GInstanceInitFunc) btk_im_context_thai_init,
  };

  btk_type_im_context_thai = 
    g_type_module_register_type (type_module,
                                 BTK_TYPE_IM_CONTEXT,
                                 "BtkIMContextThai",
                                 &im_context_thai_info, 0);
}

static void
btk_im_context_thai_class_init (BtkIMContextThaiClass *class)
{
  BtkIMContextClass *im_context_class = BTK_IM_CONTEXT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  im_context_class->filter_keypress = btk_im_context_thai_filter_keypress;
}

static void
btk_im_context_thai_init (BtkIMContextThai *context_thai)
{
#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
  forget_previous_chars (context_thai);
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */
  context_thai->isc_mode = ISC_BASICCHECK;
}

BtkIMContext *
btk_im_context_thai_new (void)
{
  BtkIMContextThai *result;

  result = BTK_IM_CONTEXT_THAI (g_object_new (BTK_TYPE_IM_CONTEXT_THAI, NULL));

  return BTK_IM_CONTEXT (result);
}

BtkIMContextThaiISCMode
btk_im_context_thai_get_isc_mode (BtkIMContextThai *context_thai)
{
  return context_thai->isc_mode;
}

BtkIMContextThaiISCMode
btk_im_context_thai_set_isc_mode (BtkIMContextThai *context_thai,
                                  BtkIMContextThaiISCMode mode)
{
  BtkIMContextThaiISCMode prev_mode = context_thai->isc_mode;
  context_thai->isc_mode = mode;
  return prev_mode;
}

static gboolean
is_context_lost_key(guint keyval)
{
  return ((keyval & 0xFF00) == 0xFF00) &&
         (keyval == BDK_BackSpace ||
          keyval == BDK_Tab ||
          keyval == BDK_Linefeed ||
          keyval == BDK_Clear ||
          keyval == BDK_Return ||
          keyval == BDK_Pause ||
          keyval == BDK_Scroll_Lock ||
          keyval == BDK_Sys_Req ||
          keyval == BDK_Escape ||
          keyval == BDK_Delete ||
          (BDK_Home <= keyval && keyval <= BDK_Begin) || /* IsCursorkey */
          (BDK_KP_Space <= keyval && keyval <= BDK_KP_Delete) || /* IsKeypadKey, non-chars only */
          (BDK_Select <= keyval && keyval <= BDK_Break) || /* IsMiscFunctionKey */
          (BDK_F1 <= keyval && keyval <= BDK_F35)); /* IsFunctionKey */
}

static gboolean
is_context_intact_key(guint keyval)
{
  return (((keyval & 0xFF00) == 0xFF00) &&
           ((BDK_Shift_L <= keyval && keyval <= BDK_Hyper_R) || /* IsModifierKey */
            (keyval == BDK_Mode_switch) ||
            (keyval == BDK_Num_Lock))) ||
         (((keyval & 0xFE00) == 0xFE00) &&
          (BDK_ISO_Lock <= keyval && keyval <= BDK_ISO_Last_Group_Lock));
}

static gboolean
thai_is_accept (gunichar new_char, gunichar prev_char, gint isc_mode)
{
  switch (isc_mode)
    {
    case ISC_PASSTHROUGH:
      return TRUE;

    case ISC_BASICCHECK:
      return TAC_compose_input (prev_char, new_char) != 'R';

    case ISC_STRICT:
      {
        int op = TAC_compose_input (prev_char, new_char);
        return op != 'R' && op != 'S';
      }

    default:
      return FALSE;
    }
}

#define thai_is_composible(n,p)  (TAC_compose_input ((p), (n)) == 'C')

#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
static void
forget_previous_chars (BtkIMContextThai *context_thai)
{
  memset (context_thai->char_buff, 0, sizeof (context_thai->char_buff));
}

static void
remember_previous_char (BtkIMContextThai *context_thai, gunichar new_char)
{
  memmove (context_thai->char_buff + 1, context_thai->char_buff,
           (BTK_IM_CONTEXT_THAI_BUFF_SIZE - 1) * sizeof (context_thai->char_buff[0]));
  context_thai->char_buff[0] = new_char;
}
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */

static gunichar
get_previous_char (BtkIMContextThai *context_thai, gint offset)
{
  gchar *surrounding;
  gint  cursor_index;

  if (btk_im_context_get_surrounding ((BtkIMContext *)context_thai,
                                      &surrounding, &cursor_index))
    {
      gunichar prev_char;
      gchar *p, *q;

      prev_char = 0;
      p = surrounding + cursor_index;
      for (q = p; offset < 0 && q > surrounding; ++offset)
        q = g_utf8_prev_char (q);
      if (offset == 0)
        {
          prev_char = g_utf8_get_char_validated (q, p - q);
          if (prev_char < 0)
            prev_char = 0;
        }
      g_free (surrounding);
      return prev_char;
    }
#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
  else
    {
      offset = -offset - 1;
      if (0 <= offset && offset < BTK_IM_CONTEXT_THAI_BUFF_SIZE)
        return context_thai->char_buff[offset];
    }
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */

    return 0;
}

static gboolean
btk_im_context_thai_commit_chars (BtkIMContextThai *context_thai,
                                  gunichar *s, gsize len)
{
  gchar *utf8;

  utf8 = g_ucs4_to_utf8 (s, len, NULL, NULL, NULL);
  if (!utf8)
    return FALSE;

  g_signal_emit_by_name (context_thai, "commit", utf8);

  g_free (utf8);
  return TRUE;
}

static gboolean
accept_input (BtkIMContextThai *context_thai, gunichar new_char)
{
#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
  remember_previous_char (context_thai, new_char);
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */

  return btk_im_context_thai_commit_chars (context_thai, &new_char, 1);
}

static gboolean
reorder_input (BtkIMContextThai *context_thai,
               gunichar prev_char, gunichar new_char)
{
  gunichar buf[2];

  if (!btk_im_context_delete_surrounding (BTK_IM_CONTEXT (context_thai), -1, 1))
    return FALSE;

#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
  forget_previous_chars (context_thai);
  remember_previous_char (context_thai, new_char);
  remember_previous_char (context_thai, prev_char);
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */

  buf[0] = new_char;
  buf[1] = prev_char;
  return btk_im_context_thai_commit_chars (context_thai, buf, 2);
}

static gboolean
replace_input (BtkIMContextThai *context_thai, gunichar new_char)
{
  if (!btk_im_context_delete_surrounding (BTK_IM_CONTEXT (context_thai), -1, 1))
    return FALSE;

#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
  forget_previous_chars (context_thai);
  remember_previous_char (context_thai, new_char);
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */

  return btk_im_context_thai_commit_chars (context_thai, &new_char, 1);
}

static gboolean
btk_im_context_thai_filter_keypress (BtkIMContext *context,
                                     BdkEventKey  *event)
{
  BtkIMContextThai *context_thai = BTK_IM_CONTEXT_THAI (context);
  gunichar prev_char, new_char;
  gboolean is_reject;
  BtkIMContextThaiISCMode isc_mode;

  if (event->type != BDK_KEY_PRESS)
    return FALSE;

  if (event->state & (BDK_MODIFIER_MASK
                      & ~(BDK_SHIFT_MASK | BDK_LOCK_MASK | BDK_MOD2_MASK)) ||
      is_context_lost_key (event->keyval))
    {
#ifndef BTK_IM_CONTEXT_THAI_NO_FALLBACK
      forget_previous_chars (context_thai);
#endif /* !BTK_IM_CONTEXT_THAI_NO_FALLBACK */
      return FALSE;
    }
  if (event->keyval == 0 || is_context_intact_key (event->keyval))
    {
      return FALSE;
    }

  prev_char = get_previous_char (context_thai, -1);
  if (!prev_char)
    prev_char = ' ';
  new_char = bdk_keyval_to_unicode (event->keyval);
  is_reject = TRUE;
  isc_mode = btk_im_context_thai_get_isc_mode (context_thai);
  if (thai_is_accept (new_char, prev_char, isc_mode))
    {
      accept_input (context_thai, new_char);
      is_reject = FALSE;
    }
  else
    {
      gunichar context_char;

      /* rejected, trying to correct */
      context_char = get_previous_char (context_thai, -2);
      if (context_char)
        {
          if (thai_is_composible (new_char, context_char))
            {
              if (thai_is_composible (prev_char, new_char))
                is_reject = !reorder_input (context_thai, prev_char, new_char);
              else if (thai_is_composible (prev_char, context_char))
                is_reject = !replace_input (context_thai, new_char);
              else if ((TAC_char_class (prev_char) == FV1
                        || TAC_char_class (prev_char) == AM)
                       && TAC_char_class (new_char) == TONE)
                is_reject = !reorder_input (context_thai, prev_char, new_char);
            }
	  else if (thai_is_accept (new_char, context_char, isc_mode))
            is_reject = !replace_input (context_thai, new_char);
        }
    }
  if (is_reject)
    {
      /* reject character */
      bdk_beep ();
    }
  return TRUE;
}

