#ifndef __BTK_OFFSCREEN_BOX_H__
#define __BTK_OFFSCREEN_BOX_H__


#include <bdk/bdk.h>
#include <btk/btk.h>


G_BEGIN_DECLS

#define BTK_TYPE_OFFSCREEN_BOX              (btk_offscreen_box_get_type ())
#define BTK_OFFSCREEN_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BTK_TYPE_OFFSCREEN_BOX, BtkOffscreenBox))
#define BTK_OFFSCREEN_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BTK_TYPE_OFFSCREEN_BOX, BtkOffscreenBoxClass))
#define BTK_IS_OFFSCREEN_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BTK_TYPE_OFFSCREEN_BOX))
#define BTK_IS_OFFSCREEN_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BTK_TYPE_OFFSCREEN_BOX))
#define BTK_OFFSCREEN_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BTK_TYPE_OFFSCREEN_BOX, BtkOffscreenBoxClass))

typedef struct _BtkOffscreenBox	  BtkOffscreenBox;
typedef struct _BtkOffscreenBoxClass  BtkOffscreenBoxClass;

struct _BtkOffscreenBox
{
  BtkContainer container;

  BtkWidget *child1;
  BtkWidget *child2;

  BdkWindow *offscreen_window1;
  BdkWindow *offscreen_window2;

  gdouble angle;
};

struct _BtkOffscreenBoxClass
{
  BtkBinClass parent_class;
};

GType	   btk_offscreen_box_get_type           (void) G_GNUC_CONST;
BtkWidget* btk_offscreen_box_new       (void);
void       btk_offscreen_box_add1      (BtkOffscreenBox *offscreen,
					BtkWidget       *child);
void       btk_offscreen_box_add2      (BtkOffscreenBox *offscreen,
					BtkWidget       *child);
void       btk_offscreen_box_set_angle (BtkOffscreenBox *offscreen,
					double           angle);



G_END_DECLS

#endif /* __BTK_OFFSCREEN_BOX_H__ */
