#include <stdio.h>
#include <btk/btk.h>
#include "btkwidgetprofiler.h"
#include "widgets.h"

#define ITERS 100000

static BtkWidget *
create_widget_cb (BtkWidgetProfiler *profiler, bpointer data)
{
  return appwindow_new ();
}

static void
report_cb (BtkWidgetProfiler *profiler, BtkWidgetProfilerReport report, BtkWidget *widget, bdouble elapsed, bpointer data)
{
  const char *type;

  switch (report) {
  case BTK_WIDGET_PROFILER_REPORT_CREATE:
    type = "widget creation";
    break;

  case BTK_WIDGET_PROFILER_REPORT_MAP:
    type = "widget map";
    break;

  case BTK_WIDGET_PROFILER_REPORT_EXPOSE:
    type = "widget expose";
    break;

  case BTK_WIDGET_PROFILER_REPORT_DESTROY:
    type = "widget destruction";
    break;

  default:
    g_assert_not_reached ();
    type = NULL;
  }

  fprintf (stdout, "%s: %g sec\n", type, elapsed);

  if (report == BTK_WIDGET_PROFILER_REPORT_DESTROY)
    fputs ("\n", stdout);
}

int
main (int argc, char **argv)
{
  BtkWidgetProfiler *profiler;

  btk_init (&argc, &argv);

  profiler = btk_widget_profiler_new ();
  g_signal_connect (profiler, "create-widget",
		    G_CALLBACK (create_widget_cb), NULL);
  g_signal_connect (profiler, "report",
		    G_CALLBACK (report_cb), NULL);

  btk_widget_profiler_set_num_iterations (profiler, ITERS);

/*   btk_widget_profiler_profile_boot (profiler); */
  btk_widget_profiler_profile_expose (profiler);
  
  return 0;
}
