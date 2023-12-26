/* Builder
 *
 * Demonstrates an interface loaded from a XML description.
 */

#include <btk/btk.h>
#include "demo-common.h"

static BtkBuilder *builder;

G_MODULE_EXPORT void
quit_activate (BtkAction *action)
{
  BtkWidget *window;

  window = BTK_WIDGET (btk_builder_get_object (builder, "window1"));
  btk_widget_destroy (window);
}

G_MODULE_EXPORT void
about_activate (BtkAction *action)
{
  BtkWidget *about_dlg;

  about_dlg = BTK_WIDGET (btk_builder_get_object (builder, "aboutdialog1"));
  btk_dialog_run (BTK_DIALOG (about_dlg));
  btk_widget_hide (about_dlg);
}

BtkWidget *
do_builder (BtkWidget *do_widget)
{
  static BtkWidget *window = NULL;
  GError *err = NULL;
  bchar *filename;
  
  if (!window)
    {
      builder = btk_builder_new ();
      filename = demo_find_file ("demo.ui", NULL);
      btk_builder_add_from_file (builder, filename, &err);
      g_free (filename);
      if (err)
	{
	  g_error ("ERROR: %s\n", err->message);
	  return NULL;
	}
      btk_builder_connect_signals (builder, NULL);
      window = BTK_WIDGET (btk_builder_get_object (builder, "window1"));
      btk_window_set_screen (BTK_WINDOW (window),
			     btk_widget_get_screen (do_widget));
      g_signal_connect (window, "destroy",
			G_CALLBACK (btk_widget_destroyed), &window);
    }

  if (!btk_widget_get_visible (window))
    {
      btk_widget_show_all (window);
    }
  else
    {	 
      btk_widget_destroy (window);
      window = NULL;
    }


  return window;
}
