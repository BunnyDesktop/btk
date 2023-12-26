#include <btk/btk.h>
#include "testlib.h"
#include <stdlib.h>

/*
 * This test modules tests the BatkImage interface. When the module is loaded
 * with testbtk , it also creates a dialog that contains BtkArrows and a 
 * BtkImage. 
 *
 */

typedef struct
{
  BtkWidget *dialog;
  BtkWidget *arrow1;
  BtkWidget *arrow2;
  BtkWidget *arrow3;
  BtkWidget *arrow4;
  BtkWidget *close_button;
  BtkImage  *image;
}MainDialog;

static void destroy (BtkWidget *widget, gpointer data)
{
  btk_widget_destroy(BTK_WIDGET(data));
}

static void _check_arrows (BatkObject *obj)
{
  BatkRole role;
  MainDialog *md;
  static gint visibleDialog = 0;


  role = batk_object_get_role(obj);
  if(role == BATK_ROLE_FRAME) {

	md = (MainDialog *) malloc (sizeof(MainDialog));
	if (visibleDialog == 0)
    {
		md->arrow1 = btk_arrow_new(BTK_ARROW_UP,BTK_SHADOW_IN);
		md->arrow2 = btk_arrow_new(BTK_ARROW_DOWN,BTK_SHADOW_IN);
		md->arrow3 = btk_arrow_new(BTK_ARROW_LEFT,BTK_SHADOW_OUT);
		md->arrow4 = btk_arrow_new(BTK_ARROW_RIGHT,BTK_SHADOW_OUT);
		md->dialog = btk_dialog_new();
		btk_window_set_modal(BTK_WINDOW(md->dialog), TRUE);
                btk_box_pack_start(BTK_BOX (BTK_DIALOG (md->dialog)->vbox),
                                   md->arrow1, TRUE,TRUE, 0);
		btk_box_pack_start(BTK_BOX (BTK_DIALOG (md->dialog)->vbox),
                                   md->arrow2, TRUE,TRUE, 0);
		btk_box_pack_start(BTK_BOX (BTK_DIALOG (md->dialog)->vbox),
                                   md->arrow3, TRUE,TRUE, 0);
		btk_box_pack_start(BTK_BOX (BTK_DIALOG (md->dialog)->vbox),
                                   md->arrow4, TRUE,TRUE, 0);
		g_signal_connect(BTK_OBJECT(md->dialog), "destroy",
                                 G_CALLBACK (destroy), md->dialog);

	        md->image = BTK_IMAGE(btk_image_new_from_file("circles.xbm"));
		btk_box_pack_start(BTK_BOX (BTK_DIALOG (md->dialog)->vbox),
                                   BTK_WIDGET(md->image), TRUE,TRUE, 0);
		md->close_button = btk_button_new_from_stock(BTK_STOCK_CLOSE);
		g_signal_connect(BTK_OBJECT(md->close_button), "clicked",
                                 G_CALLBACK (destroy), md->dialog);

		btk_box_pack_start(BTK_BOX (BTK_DIALOG (md->dialog)->action_area),
                                   md->close_button, TRUE,TRUE, 0);

		btk_widget_show_all(md->dialog);
		visibleDialog = 1;
    }
 }
}


static void 
_print_image_info(BatkObject *obj) {

  gint height, width;
  const gchar *desc;
  const gchar *name = batk_object_get_name (obj);
  const gchar *type_name = g_type_name(G_TYPE_FROM_INSTANCE (obj));

  height = width = 0;


  if(!BATK_IS_IMAGE(obj)) 
	return;

  g_print("batk_object_get_name : %s\n", name ? name : "<NULL>");
  g_print("batk_object_get_type_name : %s\n",type_name ?type_name :"<NULL>");
  g_print("*** Start Image Info ***\n");
  desc = batk_image_get_image_description(BATK_IMAGE(obj));
  g_print ("batk_image_get_image_desc returns : %s\n",desc ? desc:"<NULL>");
  batk_image_get_image_size(BATK_IMAGE(obj), &height ,&width);
  g_print("batk_image_get_image_size returns: height %d width %d\n",
											height,width);
  if(batk_image_set_image_description(BATK_IMAGE(obj),"New image Description")){
	desc = batk_image_get_image_description(BATK_IMAGE(obj));
	g_print ("batk_image_get_image_desc now returns : %s\n",desc?desc:"<NULL>");
  }
  g_print("*** End Image Info ***\n");


}
static void _traverse_children (BatkObject *obj)
{
  gint n_children, i;

  n_children = batk_object_get_n_accessible_children (obj);
  for (i = 0; i < n_children; i++)
  {
    BatkObject *child;

    child = batk_object_ref_accessible_child (obj, i);
	_print_image_info(child);
    _traverse_children (child);
    g_object_unref (G_OBJECT (child));
  }
}


static void _check_objects (BatkObject *obj)
{
  BatkRole role;

  g_print ("Start of _check_values\n");

  _check_arrows(obj);
  role = batk_object_get_role (obj);

  if (role == BATK_ROLE_FRAME || role == BATK_ROLE_DIALOG)
  {
    /*
     * Add handlers to all children.
     */
    _traverse_children (obj);
  }
  g_print ("End of _check_values\n");
}


static void
_create_event_watcher (void)
{
  batk_add_focus_tracker (_check_objects);
}

int
btk_module_init(gint argc, char* argv[])
{
  g_print("testimages Module loaded\n");

  _create_event_watcher();

  return 0;
}
