#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include <btk/btk.h>


typedef enum
{
  SMALL,
  MEDIUM,
  LARGE,
  ASIS
} WidgetSize;

typedef struct WidgetInfo
{
  BtkWidget *window;
  bchar *name;
  bboolean no_focus;
  bboolean include_decorations;
  WidgetSize size;
} WidgetInfo;

GList *get_all_widgets (void);


#endif /* __WIDGETS_H__ */
