/* testrichtext.c
 * Copyright (C) 2006 Imendio AB
 * Authors: Michael Natterer, Tim Janik
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

#include <string.h>
#include <btk/btk.h>

static buint32 quick_rand32_accu = 2147483563;

static inline buint32
quick_rand32 (void)
{
  quick_rand32_accu = 1664525 * quick_rand32_accu + 1013904223;
  return quick_rand32_accu;
}

static bboolean
delete_event (BtkWidget   *widget,
              BdkEventAny *event,
              bpointer     user_data)
{
  btk_main_quit ();

  return TRUE;
}

static void
text_tag_enqueue (BtkTextTag *tag,
                  bpointer    data)
{
  GSList **slist_p = data;
  *slist_p = b_slist_prepend (*slist_p, tag);
}

static const bchar *example_text =
"vkndsk vfds vkfds vkdsv fdlksnvkfdvnkfdvnkdsnvs\n"
"kmvofdmvfdsvkv fdskvnkfdv nnd.mckfdvnknsknvdnvs"
"fdlvmfdsvlkfdsmvnskdnvfdsnvf sbskjnvlknfd cvdvnd"
"mvlfdsv vfdkjv m, ds vkfdks v df,v j kfds v d\n"
"vnfdskv kjvnfv  cfdkvndfnvcm fd,vk kdsf vj d\n"
"KLJHkjh kjh klhjKLJH Kjh kjl h34kj h34kj3h klj 23 "
"kjlkjlhsdjk 34kljh klj hklj 23k4jkjkjh234kjh 52kj "
"2h34 sdaf ukklj kjl32l jkkjl 23j jkl ljk23 jkl\n"
"hjhjhj2hj23jh jh jk jk2h3 hj kjj jk jh21 jhhj32.";

static BdkAtom
setup_buffer (BtkTextBuffer *buffer)
{
  const buint tlen = strlen (example_text);
  const buint tcount = 17;
  BtkTextTag **tags;
  BtkTextTagTable *ttable = btk_text_buffer_get_tag_table (buffer);
  GSList *node, *slist = NULL;
  BdkAtom atom;
  buint i;

  tags = g_malloc (sizeof (BtkTextTag *) * tcount);

  /* cleanup */
  btk_text_buffer_set_text (buffer, "", 0);
  btk_text_tag_table_foreach (ttable, text_tag_enqueue, &slist);
  for (node = slist; node; node = node->next)
    btk_text_tag_table_remove (ttable, node->data);
  b_slist_free (slist);

  /* create new tags */
  for (i = 0; i < tcount; i++)
    {
      char *s = g_strdup_printf ("tag%u", i);
      tags[i] = btk_text_buffer_create_tag (buffer, s,
                                            "weight", quick_rand32() >> 31 ? BANGO_WEIGHT_BOLD : BANGO_WEIGHT_NORMAL,
                                            "style", quick_rand32() >> 31 ? BANGO_STYLE_OBLIQUE : BANGO_STYLE_NORMAL,
                                            "underline", quick_rand32() >> 31,
                                            NULL);
      g_free (s);
    }

  /* assign text and tags */
  btk_text_buffer_set_text (buffer, example_text, -1);
  for (i = 0; i < tcount * 5; i++)
    {
      bint a = quick_rand32() % tlen, b = quick_rand32() % tlen;
      BtkTextIter start, end;
      btk_text_buffer_get_iter_at_offset (buffer, &start, MIN (a, b));
      btk_text_buffer_get_iter_at_offset (buffer, &end,   MAX (a, b));
      btk_text_buffer_apply_tag (buffer, tags[i % tcount], &start, &end);
    }

  /* return serialization format */
  atom = btk_text_buffer_register_deserialize_tagset (buffer, NULL);
  btk_text_buffer_deserialize_set_can_create_tags (buffer, atom, TRUE);

  g_free (tags);

  return atom;
}

static bboolean
test_serialize_deserialize (BtkTextBuffer *buffer,
                            BdkAtom        atom,
                            GError       **error)
{
  BtkTextIter  start, end;
  buint8      *spew;
  bsize        spew_length;
  bboolean     success;

  btk_text_buffer_get_bounds (buffer, &start, &end);

  spew = btk_text_buffer_serialize (buffer, buffer, atom,
                                    &start, &end, &spew_length);

  success = btk_text_buffer_deserialize (buffer, buffer, atom, &end,
                                         spew, spew_length, error);

  g_free (spew);

  return success;
}

bint
main (bint   argc,
      bchar *argv[])
{
  BtkWidget     *window;
  BtkWidget     *sw;
  BtkWidget     *view;
  BtkTextBuffer *buffer;
  BdkAtom        atom;
  buint          i, broken = 0;

  btk_init (&argc, &argv);

  /* initialize random numbers, disable this for deterministic testing */
  if (1)        
    quick_rand32_accu = g_random_int();

  window = btk_window_new (BTK_WINDOW_TOPLEVEL);
  btk_widget_set_size_request (window, 400, 300);

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw),
                                       BTK_SHADOW_IN);
  btk_container_set_border_width (BTK_CONTAINER (sw), 12);
  btk_container_add (BTK_CONTAINER (window), sw);

  g_signal_connect (window, "delete-event",
                    G_CALLBACK (delete_event),
                    NULL);

  buffer = btk_text_buffer_new (NULL);
  view = btk_text_view_new_with_buffer (buffer);
  g_object_unref (buffer);

  btk_container_add (BTK_CONTAINER (sw), view);

  btk_widget_show_all (window);
  if (0)
    btk_main ();

  for (i = 0; i < 250; i++)
    {
      GError *error = NULL;
      g_printerr ("creating randomly tagged text buffer with accu=0x%x...\n", quick_rand32_accu);
      atom = setup_buffer (buffer);
      if (test_serialize_deserialize (buffer, atom, &error))
        g_printerr ("ok.\n");
      else
        {
          g_printerr ("FAIL: serialization/deserialization failed:\n  %s\n", error->message);
          broken += 1;
        }
      g_clear_error (&error);
    }

  return broken > 0;
}
