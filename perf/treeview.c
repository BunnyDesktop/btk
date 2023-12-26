#include <btk/btk.h>
#include "widgets.h"

struct row_data {
  char *stock_id;
  char *text1;
  char *text2;
};

static const struct row_data row_data[] = {
  { BTK_STOCK_NEW,		"First",		"Here bygynneth the Book of the tales of Caunterbury." },
  { BTK_STOCK_OPEN,		"Second",		"Whan that Aprille, with hise shoures soote," },
  { BTK_STOCK_ABOUT, 		"Third",		"The droghte of March hath perced to the roote" },
  { BTK_STOCK_ADD, 		"Fourth",		"And bathed every veyne in swich licour," },
  { BTK_STOCK_APPLY, 		"Fifth",		"Of which vertu engendred is the flour;" },
  { BTK_STOCK_BOLD, 		"Sixth",		"Whan Zephirus eek with his swete breeth" },
  { BTK_STOCK_CANCEL,		"Seventh",		"Inspired hath in every holt and heeth" },
  { BTK_STOCK_CDROM,		"Eighth",		"The tendre croppes, and the yonge sonne" },
  { BTK_STOCK_CLEAR,		"Ninth",		"Hath in the Ram his halfe cours yronne," },
  { BTK_STOCK_CLOSE,		"Tenth",		"And smale foweles maken melodye," },
  { BTK_STOCK_COLOR_PICKER,	"Eleventh",		"That slepen al the nyght with open eye-" },
  { BTK_STOCK_CONVERT,		"Twelfth",		"So priketh hem Nature in hir corages-" },
  { BTK_STOCK_CONNECT,		"Thirteenth",		"Thanne longen folk to goon on pilgrimages" },
  { BTK_STOCK_COPY,		"Fourteenth",		"And palmeres for to seken straunge strondes" },
  { BTK_STOCK_CUT,		"Fifteenth",		"To ferne halwes, kowthe in sondry londes;" },
  { BTK_STOCK_DELETE,		"Sixteenth",		"And specially, from every shires ende" },
  { BTK_STOCK_DIRECTORY,	"Seventeenth",		"Of Engelond, to Caunturbury they wende," },
  { BTK_STOCK_DISCONNECT,	"Eighteenth",		"The hooly blisful martir for the seke" },
  { BTK_STOCK_EDIT,		"Nineteenth",		"That hem hath holpen, whan that they were seeke." },
  { BTK_STOCK_EXECUTE,		"Twentieth",		"Bifil that in that seson, on a day," },
  { BTK_STOCK_FILE,		"Twenty-first",		"In Southwerk at the Tabard as I lay," },
  { BTK_STOCK_FIND,		"Twenty-second",	"Redy to wenden on my pilgrymage" },
  { BTK_STOCK_FIND_AND_REPLACE,	"Twenty-third",		"To Caunterbury, with ful devout corage," },
  { BTK_STOCK_FLOPPY,		"Twenty-fourth",	"At nyght were come into that hostelrye" },
  { BTK_STOCK_FULLSCREEN,	"Twenty-fifth",		"Wel nyne and twenty in a compaignye" },
  { BTK_STOCK_GOTO_BOTTOM,	"Twenty-sixth",		"Of sondry folk, by aventure yfalle" },
};

static BtkTreeModel *
tree_model_new (void)
{
  BtkListStore *list;
  int i;

  list = btk_list_store_new (3, B_TYPE_STRING, B_TYPE_STRING, B_TYPE_STRING);

  for (i = 0; i < G_N_ELEMENTS (row_data); i++)
    {
      BtkTreeIter iter;

      btk_list_store_append (list, &iter);
      btk_list_store_set (list,
			  &iter,
			  0, row_data[i].stock_id,
			  1, row_data[i].text1,
			  2, row_data[i].text2,
			  -1);
    }

  return BTK_TREE_MODEL (list);
}

BtkWidget *
tree_view_new (void)
{
  BtkWidget *sw;
  BtkWidget *tree;
  BtkTreeModel *model;
  BtkTreeViewColumn *column;

  sw = btk_scrolled_window_new (NULL, NULL);
  btk_scrolled_window_set_shadow_type (BTK_SCROLLED_WINDOW (sw), BTK_SHADOW_IN);

  model = tree_model_new ();
  tree = btk_tree_view_new_with_model (model);
  g_object_unref (model);

  btk_widget_set_size_request (tree, 300, 100);

  column = btk_tree_view_column_new_with_attributes ("Icon",
						     btk_cell_renderer_pixbuf_new (),
						     "stock-id", 0,
						     NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree), column);

  column = btk_tree_view_column_new_with_attributes ("Index",
						     btk_cell_renderer_text_new (),
						     "text", 1,
						     NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree), column);

  column = btk_tree_view_column_new_with_attributes ("Canterbury Tales",
						     btk_cell_renderer_text_new (),
						     "text", 2,
						     NULL);
  btk_tree_view_append_column (BTK_TREE_VIEW (tree), column);

  btk_container_add (BTK_CONTAINER (sw), tree);

  return sw;
}
