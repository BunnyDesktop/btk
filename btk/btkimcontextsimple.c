/* BTK - The GIMP Toolkit
 * Copyright (C) 2000 Red Hat, Inc.
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <bdk/bdkkeysyms.h>
#include "btkprivate.h"
#include "btkaccelgroup.h"
#include "btkimcontextsimple.h"
#include "btksettings.h"
#include "btkwidget.h"
#include "btkintl.h"
#include "btkalias.h"

#ifdef BDK_WINDOWING_WIN32
#include <win32/bdkwin32keys.h>
#endif

typedef struct _BtkComposeTable BtkComposeTable;
typedef struct _BtkComposeTableCompact BtkComposeTableCompact;

struct _BtkComposeTable 
{
  const buint16 *data;
  bint max_seq_len;
  bint n_seqs;
};

struct _BtkComposeTableCompact
{
  const buint16 *data;
  bint max_seq_len;
  bint n_index_size;
  bint n_index_stride;
};

/* This file contains the table of the compose sequences, 
 * static const buint16 btk_compose_seqs_compact[] = {}
 * IT is generated from the compose-parse.py script.
 */
#include "btkimcontextsimpleseqs.h"

/* From the values below, the value 23 means the number of different first keysyms 
 * that exist in the Compose file (from Xorg). When running compose-parse.py without 
 * parameters, you get the count that you can put here. Needed when updating the
 * btkimcontextsimpleseqs.h header file (contains the compose sequences).
 */
static const BtkComposeTableCompact btk_compose_table_compact = {
  btk_compose_seqs_compact,
  5,
  24,
  6
};

static const buint16 btk_compose_ignore[] = {
  BDK_Shift_L,
  BDK_Shift_R,
  BDK_Control_L,
  BDK_Control_R,
  BDK_Caps_Lock,
  BDK_Shift_Lock,
  BDK_Meta_L,
  BDK_Meta_R,
  BDK_Alt_L,
  BDK_Alt_R,
  BDK_Super_L,
  BDK_Super_R,
  BDK_Hyper_L,
  BDK_Hyper_R,
  BDK_Mode_switch,
  BDK_ISO_Level3_Shift
};

static void     btk_im_context_simple_finalize           (BObject                  *obj);
static bboolean btk_im_context_simple_filter_keypress    (BtkIMContext             *context,
							  BdkEventKey              *key);
static void     btk_im_context_simple_reset              (BtkIMContext             *context);
static void     btk_im_context_simple_get_preedit_string (BtkIMContext             *context,
							  bchar                   **str,
							  BangoAttrList           **attrs,
							  bint                     *cursor_pos);

G_DEFINE_TYPE (BtkIMContextSimple, btk_im_context_simple, BTK_TYPE_IM_CONTEXT)

static void
btk_im_context_simple_class_init (BtkIMContextSimpleClass *class)
{
  BtkIMContextClass *im_context_class = BTK_IM_CONTEXT_CLASS (class);
  BObjectClass *bobject_class = B_OBJECT_CLASS (class);

  im_context_class->filter_keypress = btk_im_context_simple_filter_keypress;
  im_context_class->reset = btk_im_context_simple_reset;
  im_context_class->get_preedit_string = btk_im_context_simple_get_preedit_string;
  bobject_class->finalize = btk_im_context_simple_finalize;
}

static void
btk_im_context_simple_init (BtkIMContextSimple *im_context_simple)
{  
}

static void
btk_im_context_simple_finalize (BObject *obj)
{
  BtkIMContextSimple *context_simple = BTK_IM_CONTEXT_SIMPLE (obj);

  if (context_simple->tables)
    {
      b_slist_foreach (context_simple->tables, (GFunc)g_free, NULL);
      b_slist_free (context_simple->tables);

      context_simple->tables = NULL;
    }

  B_OBJECT_CLASS (btk_im_context_simple_parent_class)->finalize (obj);
}

/** 
 * btk_im_context_simple_new:
 * 
 * Creates a new #BtkIMContextSimple.
 *
 * Returns: a new #BtkIMContextSimple.
 **/
BtkIMContext *
btk_im_context_simple_new (void)
{
  return g_object_new (BTK_TYPE_IM_CONTEXT_SIMPLE, NULL);
}

static void
btk_im_context_simple_commit_char (BtkIMContext *context,
				   gunichar ch)
{
  bchar buf[10];
  bint len;

  BtkIMContextSimple *context_simple = BTK_IM_CONTEXT_SIMPLE (context);

  g_return_if_fail (g_unichar_validate (ch));
  
  len = g_unichar_to_utf8 (ch, buf);
  buf[len] = '\0';

  if (context_simple->tentative_match || context_simple->in_hex_sequence)
    {
      context_simple->in_hex_sequence = FALSE;  
      context_simple->tentative_match = 0;
      context_simple->tentative_match_len = 0;
      g_signal_emit_by_name (context_simple, "preedit-changed");
      g_signal_emit_by_name (context_simple, "preedit-end");
    }

  g_signal_emit_by_name (context, "commit", &buf);
}

static int
compare_seq_index (const void *key, const void *value)
{
  const buint *keysyms = key;
  const buint16 *seq = value;

  if (keysyms[0] < seq[0])
    return -1;
  else if (keysyms[0] > seq[0])
    return 1;

  return 0;
}

static int
compare_seq (const void *key, const void *value)
{
  int i = 0;
  const buint *keysyms = key;
  const buint16 *seq = value;

  while (keysyms[i])
    {
      if (keysyms[i] < seq[i])
	return -1;
      else if (keysyms[i] > seq[i])
	return 1;

      i++;
    }

  return 0;
}

static bboolean
check_table (BtkIMContextSimple    *context_simple,
	     const BtkComposeTable *table,
	     bint                   n_compose)
{
  bint row_stride = table->max_seq_len + 2; 
  buint16 *seq; 
  
  /* Will never match, if the sequence in the compose buffer is longer
   * than the sequences in the table.  Further, compare_seq (key, val)
   * will overrun val if key is longer than val. */
  if (n_compose > table->max_seq_len)
    return FALSE;
  
  seq = bsearch (context_simple->compose_buffer,
		 table->data, table->n_seqs,
		 sizeof (buint16) *  row_stride, 
		 compare_seq);

  if (seq)
    {
      buint16 *prev_seq;

      /* Back up to the first sequence that matches to make sure
       * we find the exact match if their is one.
       */
      while (seq > table->data)
	{
	  prev_seq = seq - row_stride;
	  if (compare_seq (context_simple->compose_buffer, prev_seq) != 0)
	    break;
	  seq = prev_seq;
	}
      
      if (n_compose == table->max_seq_len ||
	  seq[n_compose] == 0) /* complete sequence */
	{
	  buint16 *next_seq;
	  gunichar value = 
	    0x10000 * seq[table->max_seq_len] + seq[table->max_seq_len + 1];

	  
	  /* We found a tentative match. See if there are any longer
	   * sequences containing this subsequence
	   */
	  next_seq = seq + row_stride;
	  if (next_seq < table->data + row_stride * table->n_seqs)
	    {
	      if (compare_seq (context_simple->compose_buffer, next_seq) == 0)
		{
		  context_simple->tentative_match = value;
		  context_simple->tentative_match_len = n_compose;
		
		  g_signal_emit_by_name (context_simple, "preedit-changed");

		  return TRUE;
		}
	    }

	  btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple), value);
	  context_simple->compose_buffer[0] = 0;
	}
      
      return TRUE;
    }

  return FALSE;
}

/* Checks if a keysym is a dead key. Dead key keysym values are defined in
 * ../bdk/bdkkeysyms.h and the first is BDK_dead_grave. As X.Org is updated,
 * more dead keys are added and we need to update the upper limit.
 * Currently, the upper limit is BDK_dead_dasia+1. The +1 has to do with 
 * a temporary issue in the X.Org header files. 
 * In future versions it will be just the keysym (no +1).
 */
#define IS_DEAD_KEY(k) \
    ((k) >= BDK_dead_grave && (k) <= (BDK_dead_dasia+1))

#ifdef BDK_WINDOWING_WIN32

/* On Windows, user expectation is that typing a dead accent followed
 * by space will input the corresponding spacing character. The X
 * compose tables are different for dead acute and diaeresis, which
 * when followed by space produce a plain ASCII apostrophe and double
 * quote respectively. So special-case those.
 */

static bboolean
check_win32_special_cases (BtkIMContextSimple    *context_simple,
			   bint                   n_compose)
{
  if (n_compose == 2 &&
      context_simple->compose_buffer[1] == BDK_space)
    {
      gunichar value = 0;

      switch (context_simple->compose_buffer[0])
	{
	case BDK_dead_acute:
	  value = 0x00B4; break;
	case BDK_dead_diaeresis:
	  value = 0x00A8; break;
	}
      if (value > 0)
	{
	  btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple), value);
	  context_simple->compose_buffer[0] = 0;

	  BTK_NOTE (MISC, g_print ("win32: U+%04X\n", value));
	  return TRUE;
	}
    }
  return FALSE;
}

static void
check_win32_special_case_after_compact_match (BtkIMContextSimple    *context_simple,
					      bint                   n_compose,
					      buint                  value)
{
  /* On Windows user expectation is that typing two dead accents will input
   * two corresponding spacing accents.
   */
  if (n_compose == 2 &&
      context_simple->compose_buffer[0] == context_simple->compose_buffer[1] &&
      IS_DEAD_KEY (context_simple->compose_buffer[0]))
    {
      btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple), value);
      BTK_NOTE (MISC, g_print ("win32: U+%04X ", value));
    }
}

#endif

#ifdef BDK_WINDOWING_QUARTZ

static bboolean
check_quartz_special_cases (BtkIMContextSimple *context_simple,
                            bint                n_compose)
{
  buint value = 0;

  if (n_compose == 2)
    {
      switch (context_simple->compose_buffer[0])
        {
        case BDK_KEY_dead_doubleacute:
          switch (context_simple->compose_buffer[1])
            {
            case BDK_KEY_dead_doubleacute:
            case BDK_KEY_space:
              value = BDK_KEY_quotedbl; break;

            case 'a': value = BDK_KEY_adiaeresis; break;
            case 'A': value = BDK_KEY_Adiaeresis; break;
            case 'e': value = BDK_KEY_ediaeresis; break;
            case 'E': value = BDK_KEY_Ediaeresis; break;
            case 'i': value = BDK_KEY_idiaeresis; break;
            case 'I': value = BDK_KEY_Idiaeresis; break;
            case 'o': value = BDK_KEY_odiaeresis; break;
            case 'O': value = BDK_KEY_Odiaeresis; break;
            case 'u': value = BDK_KEY_udiaeresis; break;
            case 'U': value = BDK_KEY_Udiaeresis; break;
            case 'y': value = BDK_KEY_ydiaeresis; break;
            case 'Y': value = BDK_KEY_Ydiaeresis; break;
            }
          break;

        case BDK_KEY_dead_acute:
          switch (context_simple->compose_buffer[1])
            {
            case 'c': value = BDK_KEY_ccedilla; break;
            case 'C': value = BDK_KEY_Ccedilla; break;
            }
          break;
        }
    }

  if (value > 0)
    {
      btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple),
                                         bdk_keyval_to_unicode (value));
      context_simple->compose_buffer[0] = 0;

      BTK_NOTE (MISC, g_print ("quartz: U+%04X\n", value));
      return TRUE;
    }

  return FALSE;
}

#endif

static bboolean
check_compact_table (BtkIMContextSimple    *context_simple,
	     const BtkComposeTableCompact *table,
	     bint                   n_compose)
{
  bint row_stride;
  buint16 *seq_index;
  buint16 *seq; 
  bint i;

  /* Will never match, if the sequence in the compose buffer is longer
   * than the sequences in the table.  Further, compare_seq (key, val)
   * will overrun val if key is longer than val. */
  if (n_compose > table->max_seq_len)
    return FALSE;
  
  seq_index = bsearch (context_simple->compose_buffer,
		 table->data, table->n_index_size,
		 sizeof (buint16) *  table->n_index_stride, 
		 compare_seq_index);

  if (!seq_index)
    {
      BTK_NOTE (MISC, g_print ("compact: no\n"));
      return FALSE;
    }

  if (seq_index && n_compose == 1)
    {
      BTK_NOTE (MISC, g_print ("compact: yes\n"));
      return TRUE;
    }

  BTK_NOTE (MISC, g_print ("compact: %d ", *seq_index));
  seq = NULL;

  for (i = n_compose-1; i < table->max_seq_len; i++)
    {
      row_stride = i + 1;

      if (seq_index[i+1] - seq_index[i] > 0)
        {
	  seq = bsearch (context_simple->compose_buffer + 1,
		 table->data + seq_index[i], (seq_index[i+1] - seq_index[i]) / row_stride,
		 sizeof (buint16) *  row_stride, 
		 compare_seq);

	  if (seq)
            {
              if (i == n_compose - 1)
                break;
              else
                {
                  g_signal_emit_by_name (context_simple, "preedit-changed");

		  BTK_NOTE (MISC, g_print ("yes\n"));
      		  return TRUE;
                }
             }
        }
    }

  if (!seq)
    {
      BTK_NOTE (MISC, g_print ("no\n"));
      return FALSE;
    }
  else
    {
      gunichar value;

      value = seq[row_stride - 1];

      btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple), value);
#ifdef G_OS_WIN32
      check_win32_special_case_after_compact_match (context_simple, n_compose, value);
#endif
      context_simple->compose_buffer[0] = 0;

      BTK_NOTE (MISC, g_print ("U+%04X\n", value));
      return TRUE;
    }

  BTK_NOTE (MISC, g_print ("no\n"));
  return FALSE;
}

/* This function receives a sequence of Unicode characters and tries to
 * normalize it (NFC). We check for the case the the resulting string
 * has length 1 (single character).
 * NFC normalisation normally rearranges diacritic marks, unless these
 * belong to the same Canonical Combining Class.
 * If they belong to the same canonical combining class, we produce all
 * permutations of the diacritic marks, then attempt to normalize.
 */
static bboolean
check_normalize_nfc (gunichar* combination_buffer, bint n_compose)
{
  gunichar combination_buffer_temp[BTK_MAX_COMPOSE_LEN];
  bchar *combination_utf8_temp = NULL;
  bchar *nfc_temp = NULL;
  bint n_combinations;
  gunichar temp_swap;
  bint i;

  n_combinations = 1;

  for (i = 1; i < n_compose; i++ )
     n_combinations *= i;

  /* Xorg reuses dead_tilde for the perispomeni diacritic mark.
   * We check if base character belongs to Greek Unicode block,
   * and if so, we replace tilde with perispomeni. */
  if (combination_buffer[0] >= 0x390 && combination_buffer[0] <= 0x3FF)
    {
      for (i = 1; i < n_compose; i++ )
        if (combination_buffer[i] == 0x303)
          combination_buffer[i] = 0x342;
    }

  memcpy (combination_buffer_temp, combination_buffer, BTK_MAX_COMPOSE_LEN * sizeof (gunichar) );

  for (i = 0; i < n_combinations; i++ )
    {
      g_unicode_canonical_ordering (combination_buffer_temp, n_compose);
      combination_utf8_temp = g_ucs4_to_utf8 (combination_buffer_temp, -1, NULL, NULL, NULL);
      nfc_temp = g_utf8_normalize (combination_utf8_temp, -1, G_NORMALIZE_NFC);	       	

      if (g_utf8_strlen (nfc_temp, -1) == 1)
        {
          memcpy (combination_buffer, combination_buffer_temp, BTK_MAX_COMPOSE_LEN * sizeof (gunichar) );

          g_free (combination_utf8_temp);
          g_free (nfc_temp);

          return TRUE;
        }

      g_free (combination_utf8_temp);
      g_free (nfc_temp);

      if (n_compose > 2)
        {
          temp_swap = combination_buffer_temp[i % (n_compose - 1) + 1];
          combination_buffer_temp[i % (n_compose - 1) + 1] = combination_buffer_temp[(i+1) % (n_compose - 1) + 1];
          combination_buffer_temp[(i+1) % (n_compose - 1) + 1] = temp_swap;
        }
      else
        break;
    }

  return FALSE;
}

static bboolean
check_algorithmically (BtkIMContextSimple    *context_simple,
		       bint                   n_compose)

{
  bint i;
  gunichar combination_buffer[BTK_MAX_COMPOSE_LEN];
  bchar *combination_utf8, *nfc;

  if (n_compose >= BTK_MAX_COMPOSE_LEN)
    return FALSE;

  for (i = 0; i < n_compose && IS_DEAD_KEY (context_simple->compose_buffer[i]); i++)
    ;
  if (i == n_compose)
    return TRUE;

  if (i > 0 && i == n_compose - 1)
    {
      combination_buffer[0] = bdk_keyval_to_unicode (context_simple->compose_buffer[i]);
      combination_buffer[n_compose] = 0;
      i--;
      while (i >= 0)
	{
	  switch (context_simple->compose_buffer[i])
	    {
#define CASE(keysym, unicode) \
	    case BDK_dead_##keysym: combination_buffer[i+1] = unicode; break

	    CASE (grave, 0x0300);
	    CASE (acute, 0x0301);
	    CASE (circumflex, 0x0302);
	    CASE (tilde, 0x0303);	/* Also used with perispomeni, 0x342. */
	    CASE (macron, 0x0304);
	    CASE (breve, 0x0306);
	    CASE (abovedot, 0x0307);
	    CASE (diaeresis, 0x0308);
	    CASE (hook, 0x0309);
	    CASE (abovering, 0x030A);
	    CASE (doubleacute, 0x030B);
	    CASE (caron, 0x030C);
	    CASE (abovecomma, 0x0313);         /* Equivalent to psili */
	    CASE (abovereversedcomma, 0x0314); /* Equivalent to dasia */
	    CASE (horn, 0x031B);	/* Legacy use for psili, 0x313 (or 0x343). */
	    CASE (belowdot, 0x0323);
	    CASE (cedilla, 0x0327);
	    CASE (ogonek, 0x0328);	/* Legacy use for dasia, 0x314.*/
	    CASE (iota, 0x0345);
	    CASE (voiced_sound, 0x3099);	/* Per Markus Kuhn keysyms.txt file. */
	    CASE (semivoiced_sound, 0x309A);	/* Per Markus Kuhn keysyms.txt file. */

	    /* The following cases are to be removed once xkeyboard-config,
 	     * xorg are fully updated.
 	     */
            /* Workaround for typo in 1.4.x xserver-xorg */
	    case 0xfe66: combination_buffer[i+1] = 0x314; break;
	    /* CASE (dasia, 0x314); */
	    /* CASE (perispomeni, 0x342); */
	    /* CASE (psili, 0x343); */
#undef CASE
	    default:
	      combination_buffer[i+1] = bdk_keyval_to_unicode (context_simple->compose_buffer[i]);
	    }
	  i--;
	}
      
      /* If the buffer normalizes to a single character, 
       * then modify the order of combination_buffer accordingly, if necessary,
       * and return TRUE. 
       */
      if (check_normalize_nfc (combination_buffer, n_compose))
        {
          gunichar value;
      	  combination_utf8 = g_ucs4_to_utf8 (combination_buffer, -1, NULL, NULL, NULL);
          nfc = g_utf8_normalize (combination_utf8, -1, G_NORMALIZE_NFC);

          value = g_utf8_get_char (nfc);
          btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple), value);
          context_simple->compose_buffer[0] = 0;

          g_free (combination_utf8);
          g_free (nfc);

          return TRUE;
        }
    }

  return FALSE;
}

/* In addition to the table-driven sequences, we allow Unicode hex
 * codes to be entered. The method chosen here is similar to the
 * one recommended in ISO 14755, but not exactly the same, since we
 * don't want to steal 16 valuable key combinations. 
 * 
 * A hex Unicode sequence must be started with Ctrl-Shift-U, followed
 * by a sequence of hex digits entered with Ctrl-Shift still held.
 * Releasing one of the modifiers or pressing space while the modifiers
 * are still held commits the character. It is possible to erase
 * digits using backspace.
 *
 * As an extension to the above, we also allow to start the sequence
 * with Ctrl-Shift-U, then release the modifiers before typing any
 * digits, and enter the digits without modifiers.
 */
#define HEX_MOD_MASK (BTK_DEFAULT_ACCEL_MOD_MASK | BDK_SHIFT_MASK)

static bboolean
check_hex (BtkIMContextSimple *context_simple,
           bint                n_compose)
{
  /* See if this is a hex sequence, return TRUE if so */
  bint i;
  GString *str;
  bulong n;
  bchar *nptr = NULL;
  bchar buf[7];

  context_simple->tentative_match = 0;
  context_simple->tentative_match_len = 0;

  str = g_string_new (NULL);
  
  i = 0;
  while (i < n_compose)
    {
      gunichar ch;
      
      ch = bdk_keyval_to_unicode (context_simple->compose_buffer[i]);
      
      if (ch == 0)
        return FALSE;

      if (!g_unichar_isxdigit (ch))
        return FALSE;

      buf[g_unichar_to_utf8 (ch, buf)] = '\0';

      g_string_append (str, buf);
      
      ++i;
    }

  n = strtoul (str->str, &nptr, 16);

  /* if strtoul fails it probably means non-latin digits were used;
   * we should in principle handle that, but we probably don't.
   */
  if (nptr - str->str < str->len)
    {
      g_string_free (str, TRUE);
      return FALSE;
    }
  else
    g_string_free (str, TRUE);

  if (g_unichar_validate (n))
    {
      context_simple->tentative_match = n;
      context_simple->tentative_match_len = n_compose;
    }
  
  return TRUE;
}

static void
beep_window (BdkWindow *window)
{
  BtkWidget *widget;

  bdk_window_get_user_data (window, (bpointer) &widget);

  if (BTK_IS_WIDGET (widget))
    {
      btk_widget_error_bell (widget);
    }
  else
    {
      BdkScreen *screen = bdk_window_get_screen (window);
      bboolean   beep;

      g_object_get (btk_settings_get_for_screen (screen),
                    "btk-error-bell", &beep,
                    NULL);

      if (beep)
        bdk_window_beep (window);
    }
}

static bboolean
no_sequence_matches (BtkIMContextSimple *context_simple,
                     bint                n_compose,
                     BdkEventKey        *event)
{
  BtkIMContext *context;
  gunichar ch;
  
  context = BTK_IM_CONTEXT (context_simple);
  
  /* No compose sequences found, check first if we have a partial
   * match pending.
   */
  if (context_simple->tentative_match)
    {
      bint len = context_simple->tentative_match_len;
      int i;
      
      btk_im_context_simple_commit_char (context, context_simple->tentative_match);
      context_simple->compose_buffer[0] = 0;
      
      for (i=0; i < n_compose - len - 1; i++)
	{
	  BdkEvent *tmp_event = bdk_event_copy ((BdkEvent *)event);
	  tmp_event->key.keyval = context_simple->compose_buffer[len + i];
	  
	  btk_im_context_filter_keypress (context, (BdkEventKey *)tmp_event);
	  bdk_event_free (tmp_event);
	}

      return btk_im_context_filter_keypress (context, event);
    }
  else
    {
      context_simple->compose_buffer[0] = 0;
      if (n_compose > 1)		/* Invalid sequence */
	{
	  beep_window (event->window);
	  return TRUE;
	}
  
      ch = bdk_keyval_to_unicode (event->keyval);
      if (ch != 0)
	{
	  btk_im_context_simple_commit_char (context, ch);
	  return TRUE;
	}
      else
	return FALSE;
    }
}

static bboolean
is_hex_keyval (buint keyval)
{
  gunichar ch = bdk_keyval_to_unicode (keyval);

  return g_unichar_isxdigit (ch);
}

static buint
canonical_hex_keyval (BdkEventKey *event)
{
  BdkKeymap *keymap = bdk_keymap_get_for_display (bdk_window_get_display (event->window));
  buint keyval;
  buint *keyvals = NULL;
  bint n_vals = 0;
  bint i;
  
  /* See if the keyval is already a hex digit */
  if (is_hex_keyval (event->keyval))
    return event->keyval;

  /* See if this key would have generated a hex keyval in
   * any other state, and return that hex keyval if so
   */
  bdk_keymap_get_entries_for_keycode (keymap,
				      event->hardware_keycode,
				      NULL,
				      &keyvals, &n_vals);

  keyval = 0;
  i = 0;
  while (i < n_vals)
    {
      if (is_hex_keyval (keyvals[i]))
        {
          keyval = keyvals[i];
          break;
        }

      ++i;
    }

  g_free (keyvals);
  
  if (keyval)
    return keyval;
  else
    /* No way to make it a hex digit
     */
    return 0;
}

static bboolean
btk_im_context_simple_filter_keypress (BtkIMContext *context,
				       BdkEventKey  *event)
{
  BtkIMContextSimple *context_simple = BTK_IM_CONTEXT_SIMPLE (context);
  GSList *tmp_list;  
  int n_compose = 0;
  bboolean have_hex_mods;
  bboolean is_hex_start;
  bboolean is_hex_end;
  bboolean is_backspace;
  bboolean is_escape;
  buint hex_keyval;
  int i;

  while (context_simple->compose_buffer[n_compose] != 0)
    n_compose++;

  if (event->type == BDK_KEY_RELEASE)
    {
      if (context_simple->in_hex_sequence &&
	  (event->keyval == BDK_Control_L || event->keyval == BDK_Control_R ||
	   event->keyval == BDK_Shift_L || event->keyval == BDK_Shift_R))
	{
	  if (context_simple->tentative_match &&
	      g_unichar_validate (context_simple->tentative_match))
	    {
	      btk_im_context_simple_commit_char (context, context_simple->tentative_match);
	      context_simple->compose_buffer[0] = 0;

	    }
	  else if (n_compose == 0)
	    {
	      context_simple->modifiers_dropped = TRUE;
	    }
	  else
	    {
	      /* invalid hex sequence */
	      beep_window (event->window);
	      
	      context_simple->tentative_match = 0;
	      context_simple->in_hex_sequence = FALSE;
	      context_simple->compose_buffer[0] = 0;
	      
	      g_signal_emit_by_name (context_simple, "preedit-changed");
	      g_signal_emit_by_name (context_simple, "preedit-end");
	    }

	  return TRUE;
	}
      else
	return FALSE;
    }

  /* Ignore modifier key presses */
  for (i = 0; i < G_N_ELEMENTS (btk_compose_ignore); i++)
    if (event->keyval == btk_compose_ignore[i])
      return FALSE;

  if (context_simple->in_hex_sequence && context_simple->modifiers_dropped)
    have_hex_mods = TRUE;
  else
    have_hex_mods = (event->state & (HEX_MOD_MASK)) == HEX_MOD_MASK;
  is_hex_start = event->keyval == BDK_U;
  is_hex_end = (event->keyval == BDK_space || 
		event->keyval == BDK_KP_Space ||
		event->keyval == BDK_Return || 
		event->keyval == BDK_ISO_Enter ||
		event->keyval == BDK_KP_Enter);
  is_backspace = event->keyval == BDK_BackSpace;
  is_escape = event->keyval == BDK_Escape;
  hex_keyval = canonical_hex_keyval (event);

  /* If we are already in a non-hex sequence, or
   * this keystroke is not hex modifiers + hex digit, don't filter
   * key events with accelerator modifiers held down. We only treat
   * Control and Alt as accel modifiers here, since Super, Hyper and
   * Meta are often co-located with Mode_Switch, Multi_Key or
   * ISO_Level3_Switch.
   */
  if (!have_hex_mods ||
      (n_compose > 0 && !context_simple->in_hex_sequence) || 
      (n_compose == 0 && !context_simple->in_hex_sequence && !is_hex_start) ||
      (context_simple->in_hex_sequence && !hex_keyval && 
       !is_hex_start && !is_hex_end && !is_escape && !is_backspace))
    {
      if (event->state & BTK_NO_TEXT_INPUT_MOD_MASK ||
	  (context_simple->in_hex_sequence && context_simple->modifiers_dropped &&
	   (event->keyval == BDK_Return || 
	    event->keyval == BDK_ISO_Enter ||
	    event->keyval == BDK_KP_Enter)))
	{
	  return FALSE;
	}
    }
  
  /* Handle backspace */
  if (context_simple->in_hex_sequence && have_hex_mods && is_backspace)
    {
      if (n_compose > 0)
	{
	  n_compose--;
	  context_simple->compose_buffer[n_compose] = 0;
          check_hex (context_simple, n_compose);
	}
      else
	{
	  context_simple->in_hex_sequence = FALSE;
	}

      g_signal_emit_by_name (context_simple, "preedit-changed");

      if (!context_simple->in_hex_sequence)
        g_signal_emit_by_name (context_simple, "preedit-end");
      
      return TRUE;
    }

  /* Check for hex sequence restart */
  if (context_simple->in_hex_sequence && have_hex_mods && is_hex_start)
    {
      if (context_simple->tentative_match &&
	  g_unichar_validate (context_simple->tentative_match))
	{
	  btk_im_context_simple_commit_char (context, context_simple->tentative_match);
	  context_simple->compose_buffer[0] = 0;
	}
      else 
	{
	  /* invalid hex sequence */
	  if (n_compose > 0)
	    beep_window (event->window);
	  
	  context_simple->tentative_match = 0;
	  context_simple->in_hex_sequence = FALSE;
	  context_simple->compose_buffer[0] = 0;
	}
    }
  
  /* Check for hex sequence start */
  if (!context_simple->in_hex_sequence && have_hex_mods && is_hex_start)
    {
      context_simple->compose_buffer[0] = 0;
      context_simple->in_hex_sequence = TRUE;
      context_simple->modifiers_dropped = FALSE;
      context_simple->tentative_match = 0;

      g_signal_emit_by_name (context_simple, "preedit-start");
      g_signal_emit_by_name (context_simple, "preedit-changed");
  
      return TRUE;
    }
  
  /* Then, check for compose sequences */
  if (context_simple->in_hex_sequence)
    {
      if (hex_keyval)
	context_simple->compose_buffer[n_compose++] = hex_keyval;
      else if (is_escape)
	{
	  btk_im_context_simple_reset (context);
	  
	  return TRUE;
	}
      else if (!is_hex_end)
	{
	  /* non-hex character in hex sequence */
	  beep_window (event->window);
	  
	  return TRUE;
	}
    }
  else
    context_simple->compose_buffer[n_compose++] = event->keyval;

  context_simple->compose_buffer[n_compose] = 0;

  if (context_simple->in_hex_sequence)
    {
      /* If the modifiers are still held down, consider the sequence again */
      if (have_hex_mods)
        {
          /* space or return ends the sequence, and we eat the key */
          if (n_compose > 0 && is_hex_end)
            {
	      if (context_simple->tentative_match &&
		  g_unichar_validate (context_simple->tentative_match))
		{
		  btk_im_context_simple_commit_char (context, context_simple->tentative_match);
		  context_simple->compose_buffer[0] = 0;
		}
	      else
		{
		  /* invalid hex sequence */
		  beep_window (event->window);

		  context_simple->tentative_match = 0;
		  context_simple->in_hex_sequence = FALSE;
		  context_simple->compose_buffer[0] = 0;
		}
            }
          else if (!check_hex (context_simple, n_compose))
	    beep_window (event->window);
	  
	  g_signal_emit_by_name (context_simple, "preedit-changed");

	  if (!context_simple->in_hex_sequence)
	    g_signal_emit_by_name (context_simple, "preedit-end");

	  return TRUE;
        }
    }
  else
    {
#ifdef BDK_WINDOWING_WIN32
      buint16  output[2];
      bsize    output_size = 2;

      switch (bdk_win32_keymap_check_compose (BDK_WIN32_KEYMAP (bdk_keymap_get_default ()),
                                              context_simple->compose_buffer,
                                              n_compose,
                                              output, &output_size))
        {
        case BDK_WIN32_KEYMAP_MATCH_NONE:
          break;
        case BDK_WIN32_KEYMAP_MATCH_EXACT:
        case BDK_WIN32_KEYMAP_MATCH_PARTIAL:
          for (i = 0; i < output_size; i++)
            {
              buint32 output_char = bdk_keyval_to_unicode (output[i]);
              btk_im_context_simple_commit_char (BTK_IM_CONTEXT (context_simple),
                                                 output_char);
            }
          context_simple->compose_buffer[0] = 0;
          return TRUE;
        case BDK_WIN32_KEYMAP_MATCH_INCOMPLETE:
          return TRUE;
        }
#endif


      tmp_list = context_simple->tables;
      while (tmp_list)
        {
          if (check_table (context_simple, tmp_list->data, n_compose))
            return TRUE;
          tmp_list = tmp_list->next;
        }

      BTK_NOTE (MISC, {
	  g_print ("[ ");
	  for (i = 0; i < n_compose; i++)
	    {
	      const bchar *keyval_name = bdk_keyval_name (context_simple->compose_buffer[i]);
	      
	      if (keyval_name != NULL)
		g_print ("%s ", keyval_name);
	      else
		g_print ("%04x ", context_simple->compose_buffer[i]);
	    }
	  g_print ("] ");
	});

#ifdef BDK_WINDOWING_WIN32
      if (check_win32_special_cases (context_simple, n_compose))
	return TRUE;
#endif

#ifdef BDK_WINDOWING_QUARTZ
      if (check_quartz_special_cases (context_simple, n_compose))
        return TRUE;
#endif

      if (check_compact_table (context_simple, &btk_compose_table_compact, n_compose))
        return TRUE;
  
      if (check_algorithmically (context_simple, n_compose))
	return TRUE;
    }
  
  /* The current compose_buffer doesn't match anything */
  return no_sequence_matches (context_simple, n_compose, event);
}

static void
btk_im_context_simple_reset (BtkIMContext *context)
{
  BtkIMContextSimple *context_simple = BTK_IM_CONTEXT_SIMPLE (context);

  context_simple->compose_buffer[0] = 0;

  if (context_simple->tentative_match || context_simple->in_hex_sequence)
    {
      context_simple->in_hex_sequence = FALSE;
      context_simple->tentative_match = 0;
      context_simple->tentative_match_len = 0;
      g_signal_emit_by_name (context_simple, "preedit-changed");
      g_signal_emit_by_name (context_simple, "preedit-end");
    }
}

static void     
btk_im_context_simple_get_preedit_string (BtkIMContext   *context,
					  bchar         **str,
					  BangoAttrList **attrs,
					  bint           *cursor_pos)
{
  char outbuf[37]; /* up to 6 hex digits */
  int len = 0;
  
  BtkIMContextSimple *context_simple = BTK_IM_CONTEXT_SIMPLE (context);

  if (context_simple->in_hex_sequence)
    {
      int hexchars = 0;
         
      outbuf[0] = 'u';
      len = 1;

      while (context_simple->compose_buffer[hexchars] != 0)
	{
	  len += g_unichar_to_utf8 (bdk_keyval_to_unicode (context_simple->compose_buffer[hexchars]),
				    outbuf + len);
	  ++hexchars;
	}

      g_assert (len < 25);
    }
  else if (context_simple->tentative_match)
    len = g_unichar_to_utf8 (context_simple->tentative_match, outbuf);
      
  outbuf[len] = '\0';      

  if (str)
    *str = g_strdup (outbuf);

  if (attrs)
    {
      *attrs = bango_attr_list_new ();
      
      if (len)
	{
	  BangoAttribute *attr = bango_attr_underline_new (BANGO_UNDERLINE_SINGLE);
	  attr->start_index = 0;
          attr->end_index = len;
	  bango_attr_list_insert (*attrs, attr);
	}
    }

  if (cursor_pos)
    *cursor_pos = len;
}

/**
 * btk_im_context_simple_add_table:
 * @context_simple: A #BtkIMContextSimple
 * @data: the table 
 * @max_seq_len: Maximum length of a sequence in the table
 *               (cannot be greater than #BTK_MAX_COMPOSE_LEN)
 * @n_seqs: number of sequences in the table
 * 
 * Adds an additional table to search to the input context.
 * Each row of the table consists of @max_seq_len key symbols
 * followed by two #buint16 interpreted as the high and low
 * words of a #gunicode value. Tables are searched starting
 * from the last added.
 *
 * The table must be sorted in dictionary order on the
 * numeric value of the key symbol fields. (Values beyond
 * the length of the sequence should be zero.)
 **/
void
btk_im_context_simple_add_table (BtkIMContextSimple *context_simple,
				 buint16            *data,
				 bint                max_seq_len,
				 bint                n_seqs)
{
  BtkComposeTable *table;

  g_return_if_fail (BTK_IS_IM_CONTEXT_SIMPLE (context_simple));
  g_return_if_fail (data != NULL);
  g_return_if_fail (max_seq_len <= BTK_MAX_COMPOSE_LEN);
  
  table = g_new (BtkComposeTable, 1);
  table->data = data;
  table->max_seq_len = max_seq_len;
  table->n_seqs = n_seqs;

  context_simple->tables = b_slist_prepend (context_simple->tables, table);
}

#define __BTK_IM_CONTEXT_SIMPLE_C__
#include "btkaliasdef.c"
