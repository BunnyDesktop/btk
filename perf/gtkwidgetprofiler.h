#include <btk/btk.h>

#ifndef BTK_WIDGET_PROFILER_H
#define BTK_WIDGET_PROFILER_H

G_BEGIN_DECLS

#define BTK_TYPE_WIDGET_PROFILER		(btk_widget_profiler_get_type ())
#define BTK_WIDGET_PROFILER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_WIDGET_PROFILER, BtkWidgetProfiler))
#define BTK_WIDGET_PROFILER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_WIDGET_PROFILER, BtkWidgetProfilerClass))
#define BTK_IS_WIDGET_PROFILER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_WIDGET_PROFILER))
#define BTK_IS_WIDGET_PROFILER_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_WIDGET_PROFILER))
#define BTK_WIDGET_PROFILER_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_WIDGET_PROFILER, BtkWidgetProfilerClass))

typedef enum
{
  BTK_WIDGET_PROFILER_REPORT_CREATE,
  BTK_WIDGET_PROFILER_REPORT_MAP,
  BTK_WIDGET_PROFILER_REPORT_EXPOSE,
  BTK_WIDGET_PROFILER_REPORT_DESTROY
} BtkWidgetProfilerReport;

typedef struct _BtkWidgetProfiler BtkWidgetProfiler;
typedef struct _BtkWidgetProfilerClass BtkWidgetProfilerClass;
typedef struct _BtkWidgetProfilerPrivate BtkWidgetProfilerPrivate;

struct _BtkWidgetProfiler {
	GObject object;

	BtkWidgetProfilerPrivate *priv;
};

struct _BtkWidgetProfilerClass {
	GObjectClass parent_class;

	/* signals */

	BtkWidget *(* create_widget) (BtkWidgetProfiler *profiler);

	void (* report) (BtkWidgetProfiler      *profiler,
			 BtkWidgetProfilerReport report,
			 BtkWidget              *widget,
			 gdouble                 elapsed);
};

GType btk_widget_profiler_get_type (void) G_GNUC_CONST;

BtkWidgetProfiler *btk_widget_profiler_new (void);

void btk_widget_profiler_set_num_iterations (BtkWidgetProfiler *profiler,
					     gint               n_iterations);

void btk_widget_profiler_profile_boot (BtkWidgetProfiler *profiler);

void btk_widget_profiler_profile_expose (BtkWidgetProfiler *profiler);


G_END_DECLS

#endif
