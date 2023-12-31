/*
 * Copyright (c) 2006-2009 Openismus GmbH
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

#include "btkimcontextmultipress.h"
#include <string.h>
#include <btk/btk.h>
#include <bdk/bdkkeysyms.h>
#include <btk/btkimmodule.h>
#include <config.h>

#define AUTOMATIC_COMPOSE_TIMEOUT 1 /* seconds */
#define CONFIGURATION_FILENAME MULTIPRESS_CONFDIR G_DIR_SEPARATOR_S "im-multipress.conf"

/* This contains rows of characters that can be entered by pressing
 * a particular key repeatedly.  Each row has one key (such as BDK_a),
 * and an array of character strings, such as "a".
 */
typedef struct
{
  bchar **characters; /* array of strings */
  bsize n_characters; /* number of strings in the array */
}
KeySequence;

static BObjectClass *im_context_multipress_parent_class = NULL;
static GType         im_context_multipress_type = 0;

static void im_context_multipress_class_init (BtkImContextMultipressClass *klass);
static void im_context_multipress_init (BtkImContextMultipress *self);
static void im_context_multipress_finalize (BObject *obj);

static void load_config (BtkImContextMultipress *self);

static bboolean vfunc_filter_keypress (BtkIMContext *context,
                                       BdkEventKey  *event);
static void vfunc_reset (BtkIMContext *context);
static void vfunc_get_preedit_string (BtkIMContext   *context,
                                      bchar         **str,
                                      BangoAttrList **attrs,
                                      bint           *cursor_pos);

/* Notice that we have a *_register_type(GTypeModule*) function instead of a
 * *_get_type() function, because we must use g_type_module_register_type(),
 * providing the GTypeModule* that was provided to im_context_init(). That
 * is also why we are not using G_DEFINE_TYPE().
 */
void
btk_im_context_multipress_register_type (GTypeModule* type_module)
{
  const GTypeInfo im_context_multipress_info =
    {
      sizeof (BtkImContextMultipressClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) &im_context_multipress_class_init,
      NULL,
      NULL,
      sizeof (BtkImContextMultipress),
      0,
      (GInstanceInitFunc) &im_context_multipress_init,
      0,
    };

  im_context_multipress_type =
    g_type_module_register_type (type_module,
                                 BTK_TYPE_IM_CONTEXT,
                                 "BtkImContextMultipress",
                                 &im_context_multipress_info, 0);
}

GType
btk_im_context_multipress_get_type (void)
{
  g_assert (im_context_multipress_type != 0);

  return im_context_multipress_type;
}

static void
key_sequence_free (bpointer value)
{
  KeySequence *seq = value;

  if (seq != NULL)
    {
      g_strfreev (seq->characters);
      g_slice_free (KeySequence, seq);
    }
}

static void
im_context_multipress_class_init (BtkImContextMultipressClass *klass)
{
  BtkIMContextClass *im_context_class;

  /* Set this so we can use it later: */
  im_context_multipress_parent_class = g_type_class_peek_parent (klass);

  /* Specify our vfunc implementations: */
  im_context_class = BTK_IM_CONTEXT_CLASS (klass);
  im_context_class->filter_keypress = &vfunc_filter_keypress;
  im_context_class->reset = &vfunc_reset;
  im_context_class->get_preedit_string = &vfunc_get_preedit_string;

  B_OBJECT_CLASS (klass)->finalize = &im_context_multipress_finalize;
}

static void
im_context_multipress_init (BtkImContextMultipress *self)
{
  self->key_sequences = g_hash_table_new_full (&g_direct_hash, &g_direct_equal,
                                               NULL, &key_sequence_free);
  load_config (self);
}

static void
im_context_multipress_finalize (BObject *obj)
{
  BtkImContextMultipress *self;

  self = BTK_IM_CONTEXT_MULTIPRESS (obj);

  /* Release the configuration data: */
  if (self->key_sequences != NULL)
    {
      g_hash_table_destroy (self->key_sequences);
      self->key_sequences = NULL;
    }

  (*im_context_multipress_parent_class->finalize) (obj);
}


BtkIMContext *
btk_im_context_multipress_new (void)
{
  return (BtkIMContext *)g_object_new (BTK_TYPE_IM_CONTEXT_MULTIPRESS, NULL);
}

static void
cancel_automatic_timeout_commit (BtkImContextMultipress *multipress_context)
{
  if (multipress_context->timeout_id)
    g_source_remove (multipress_context->timeout_id);
 
  multipress_context->timeout_id = 0;
}


/* Clear the compose buffer, so we are ready to compose the next character.
 */
static void
clear_compose_buffer (BtkImContextMultipress *multipress_context)
{
  multipress_context->key_last_entered = 0;
  multipress_context->compose_count = 0;

  cancel_automatic_timeout_commit (multipress_context);

  if (multipress_context->tentative_match)
    {
      multipress_context->tentative_match = NULL;
      g_signal_emit_by_name (multipress_context, "preedit-changed");
      g_signal_emit_by_name (multipress_context, "preedit-end");
    }
}

/* Finish composing, provide the character, and clear our compose buffer.
 */
static void
accept_character (BtkImContextMultipress *multipress_context, const bchar *characters)
{
  /* Clear the compose buffer, so we are ready to compose the next character.
   * Note that if we emit "preedit-changed" after "commit", there's a segfault/
   * invalid-write with BtkTextView in btk_text_layout_free_line_display(), when
   * destroying a BangoLayout (this can also be avoided by not using any Bango
   * attributes in get_preedit_string(). */
  clear_compose_buffer (multipress_context);

  /* Provide the character to BTK+ */
  g_signal_emit_by_name (multipress_context, "commit", characters);
}

static bboolean
on_timeout (bpointer data)
{
  BtkImContextMultipress *multipress_context;

  BDK_THREADS_ENTER ();

  multipress_context = BTK_IM_CONTEXT_MULTIPRESS (data);

  /* A certain amount of time has passed, so we will assume that the user
   * really wants the currently chosen character */
  accept_character (multipress_context, multipress_context->tentative_match);

  multipress_context->timeout_id = 0;

  BDK_THREADS_LEAVE ();

  return FALSE; /* don't call me again */
}

static bboolean
vfunc_filter_keypress (BtkIMContext *context, BdkEventKey *event)
{
  BtkIMContextClass      *parent;
  BtkImContextMultipress *multipress_context;

  multipress_context = BTK_IM_CONTEXT_MULTIPRESS (context);

  if (event->type == BDK_KEY_PRESS)
    {
      KeySequence *possible;

      /* Check whether the current key is the same as previously entered, because
       * if it is not then we should accept the previous one, and start a new
       * character. */
      if (multipress_context->compose_count > 0
          && multipress_context->key_last_entered != event->keyval
          && multipress_context->tentative_match != NULL)
        {
          /* Accept the previously chosen character.  This wipes
           * the compose_count and key_last_entered. */
          accept_character (multipress_context,
                            multipress_context->tentative_match);
        } 

      /* Decide what character this key press would choose: */
      possible = g_hash_table_lookup (multipress_context->key_sequences,
                                      BUINT_TO_POINTER (event->keyval));
      if (possible != NULL)
        {
          if (multipress_context->compose_count == 0)
            g_signal_emit_by_name (multipress_context, "preedit-start");

          /* Check whether we are at the end of a compose sequence, with no more
           * possible characters.  Cycle back to the start if necessary. */
          if (multipress_context->compose_count >= possible->n_characters)
            multipress_context->compose_count = 0;

          /* Store the last key pressed in the compose sequence. */
          multipress_context->key_last_entered = event->keyval; 

          /* Get the possible match for this number of presses of the key.
           * compose_count starts at 1, so that 0 can mean not composing. */ 
          multipress_context->tentative_match =
            possible->characters[multipress_context->compose_count++];

          /* Indicate the current possible character.  This will cause our
           * vfunc_get_preedit_string() vfunc to be called, which will provide
           * the current possible character for the user to see. */
          g_signal_emit_by_name (multipress_context, "preedit-changed");

          /* Cancel any outstanding timeout, so we can start the timer again: */
          cancel_automatic_timeout_commit (multipress_context);

          /* Create a timeout that will cause the currently chosen character to
           * be committed, if nothing happens for a certain amount of time: */
          multipress_context->timeout_id =
            g_timeout_add_seconds (AUTOMATIC_COMPOSE_TIMEOUT,
                                   &on_timeout, multipress_context);

          return TRUE; /* key handled */
        }
      else
        {
          buint32 keyval_uchar;

          /* Just accept all other keypresses directly, but commit the
           * current preedit content first. */
          if (multipress_context->compose_count > 0
              && multipress_context->tentative_match != NULL)
            {
              accept_character (multipress_context,
                                multipress_context->tentative_match);
            }
          keyval_uchar = bdk_keyval_to_unicode (event->keyval);

          /* Convert to a string for accept_character(). */
          if (keyval_uchar != 0)
            {
              /* max length of UTF-8 sequence = 6 + 1 for NUL termination */
              bchar keyval_utf8[7];
              bint  length;

              length = g_unichar_to_utf8 (keyval_uchar, keyval_utf8);
              keyval_utf8[length] = '\0';

              accept_character (multipress_context, keyval_utf8);

              return TRUE; /* key handled */
            }
        }
    }

  parent = (BtkIMContextClass *)im_context_multipress_parent_class;

  /* The default implementation just returns FALSE, but it is generally
   * a good idea to call the base class implementation: */
  if (parent->filter_keypress)
    return (*parent->filter_keypress) (context, event);

  return FALSE;
}

static void
vfunc_reset (BtkIMContext *context)
{
  clear_compose_buffer (BTK_IM_CONTEXT_MULTIPRESS (context));
}

static void
vfunc_get_preedit_string (BtkIMContext   *context,
                          bchar         **str,
                          BangoAttrList **attrs,
                          bint           *cursor_pos)
{
  bsize len_bytes = 0;
  bsize len_utf8_chars = 0;

  /* Show the user what character he will get if he accepts: */
  if (str != NULL)
    {
      const bchar *match;

      match = BTK_IM_CONTEXT_MULTIPRESS (context)->tentative_match;

      if (match == NULL)
        match = ""; /* *str must not be NUL */

      len_bytes = strlen (match); /* byte count */
      len_utf8_chars = g_utf8_strlen (match, len_bytes); /* character count */

      *str = g_strndup (match, len_bytes);
    }

  /* Underline it, to show the user that he is in compose mode: */
  if (attrs != NULL)
    {
      *attrs = bango_attr_list_new ();

      if (len_bytes > 0)
        {
          BangoAttribute *attr;

          attr = bango_attr_underline_new (BANGO_UNDERLINE_SINGLE);
          attr->start_index = 0;
          attr->end_index = len_bytes;
          bango_attr_list_insert (*attrs, attr);
        }
    }

  if (cursor_pos)
    *cursor_pos = len_utf8_chars;
}

/* Open the configuration file and fill in the key_sequences hash table
 * with key/character-list pairs taken from the [keys] group of the file.
 */
static void
load_config (BtkImContextMultipress *self)
{
  GKeyFile *key_file;
  GError   *error = NULL;
  bchar   **keys;
  bsize     n_keys = 0;
  bsize     i;

  key_file = g_key_file_new ();

  if (!g_key_file_load_from_file (key_file, CONFIGURATION_FILENAME,
                                  G_KEY_FILE_NONE, &error))
    {
      g_warning ("Error while trying to open the %s configuration file: %s",
                 CONFIGURATION_FILENAME, error->message);
      g_error_free (error);
      g_key_file_free (key_file);
      return;
    }

  keys = g_key_file_get_keys (key_file, "keys", &n_keys, &error);

  if (error != NULL)
    {
      g_warning ("Error while trying to read the %s configuration file: %s",
                 CONFIGURATION_FILENAME, error->message);
      g_error_free (error);
      g_key_file_free (key_file);
      return;
    }

  for (i = 0; i < n_keys; ++i)
    {
      KeySequence *seq;
      buint        keyval;

      keyval = bdk_keyval_from_name (keys[i]);

      if (keyval == BDK_VoidSymbol)
        {
          g_warning ("Error while trying to read the %s configuration file: "
                     "invalid key name \"%s\"",
                     CONFIGURATION_FILENAME, keys[i]);
          continue;
        }

      seq = g_slice_new (KeySequence);
      seq->characters = g_key_file_get_string_list (key_file, "keys", keys[i],
                                                    &seq->n_characters, &error);
      if (error != NULL)
        {
          g_warning ("Error while trying to read the %s configuration file: %s",
                     CONFIGURATION_FILENAME, error->message);
          g_error_free (error);
          error = NULL;
          g_slice_free (KeySequence, seq);
          continue;
        }

      /* Ownership of the KeySequence is taken over by the hash table */
      g_hash_table_insert (self->key_sequences, BUINT_TO_POINTER (keyval), seq);
    }

  g_strfreev (keys);
  g_key_file_free (key_file);
}
