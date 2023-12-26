#define MAX_BUFFER 256
#undef BTK_DISABLE_DEPRECATED
#define BTK_ENABLE_BROKEN
#define MAX_GROUPS 20
#define MAX_NAME_VALUE 20

#include "config.h"

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "testlib.h"

typedef enum
{
  OBJECT,
  ACTION,
  COMPONENT,
  IMAGE,
  SELECTION,
  TABLE,
  TEXT,
  VALUE,
  END_TABS
} TabNumber;

typedef enum
{
  OBJECT_INTERFACE,
  RELATION_INTERFACE,
  STATE_INTERFACE,
  ACTION_INTERFACE,
  COMPONENT_INTERFACE,
  IMAGE_INTERFACE,
  SELECTION_INTERFACE,
  TABLE_INTERFACE,
  TEXT_INTERFACE,
  TEXT_ATTRIBUTES,
  VALUE_INTERFACE
} GroupId;

typedef enum
{
  VALUE_STRING,
  VALUE_BOOLEAN,
  VALUE_TEXT,
  VALUE_BUTTON
} ValueType;

/* GUI Information for the group */

typedef struct
{
  GroupId       group_id;
  BtkFrame      *scroll_outer_frame;
  BtkWidget     *frame;
  BtkVBox       *group_vbox;
  BtkAdjustment *adj;
  GList         *name_value;
  gchar         *name;
  gboolean      is_scrolled;
  gint          default_height;
} GroupInfo;

typedef struct
{
  GList     *groups;
  BtkWidget *page;
  BtkWidget *main_box;
  gchar     *name;
} TabInfo;

typedef struct
{
  ValueType type;
  gboolean  active;

  BtkHBox *column1, *column2, *hbox;
  BtkLabel *label;

  BtkButton *button;
  BValue    button_gval;
  gulong    signal_id;
  BatkObject *batkobj;
  gint      action_num;

  BtkWidget *string;
  BtkWidget *boolean;
  BtkWidget *text;
} NameValue;

typedef enum {
   FERRET_SIGNAL_OBJECT,
   FERRET_SIGNAL_TEXT,
   FERRET_SIGNAL_TABLE
} FerretSignalType;

/* Function prototypes */

/* GUI functions */

static void _init_data(void);
static void _create_window(void);
static void _add_menu(BtkWidget ** menu, BtkWidget ** menuitem,
  gchar * name, gboolean init_value, GCallback func);
static void _clear_tab(TabNumber tab_n);
static void _greyout_tab (BtkWidget *widget, gboolean is_sensitive);
static void _finished_group(TabNumber tab_n, gint group_num);
static gboolean _object_is_ours (BatkObject *aobject);
static void _create_event_watcher (void);

/* Mouse Watcher/Magnifier/Festival functions */

static gboolean _mouse_watcher (GSignalInvocationHint *ihint,
	guint                  n_param_values,
	const BValue          *param_values,
	gpointer               data);
static gboolean _button_watcher (GSignalInvocationHint *ihint,
	guint                  n_param_values,
	const BValue          *param_values,
	gpointer               data);
static void _send_to_magnifier (gint x, gint y, gint w, gint h);
static void _send_to_festival (const gchar * name,
  const gchar * role_name, const gchar * accel);
static void _speak_caret_event (BatkObject * aobject);
static void _festival_say (const gchar * text);
static void _festival_write (const gchar * text, int fd);
static gint _festival_init (void);

/* Update functions */

static void _update_current_page(BtkNotebook *notebook, gpointer p,
  guint current_page);
static void _update(TabNumber top_tab, BatkObject *aobject);

/* Print functions */

static void _print_accessible (BatkObject *aobject);

static gint _print_object (BatkObject *aobject);
static gint _print_relation (BatkObject *aobject);
static gint _print_state (BatkObject *aobject);

static gint _print_action (BatkAction *aobject);
static gint _print_component (BatkComponent *aobject);
static gint _print_image (BatkImage *aobject);
static gint _print_selection (BatkSelection *aobject);
static gint _print_table (BatkTable *aobject);
static gint _print_text (BatkText *aobject);
static gint _print_text_attributes (BatkText *aobject);
static gint _print_value (BatkValue *aobject);
static void _print_value_type(gint group_num, gchar *type, BValue *value);
static gint _print_groupname(TabNumber tab_n, GroupId group_id,
  const char *groupname);
static NameValue* _print_key_value(TabNumber tab_n, gint group_number,
  const char *label, gpointer value, ValueType type);
static void _print_signal(BatkObject *aobject, FerretSignalType type,
  const char *name, const char *info);

/* Data Access functions */

static GroupInfo* _get_group(TabInfo *tab, GroupId group_id,
  const gchar *groupname);
void _get_group_scrolled(GroupInfo *group);
static NameValue* _get_name_value(GroupInfo *group, const gchar *label,
  gpointer value, ValueType type);

/* Signal handlers */

static void _update_handlers(BatkObject *obj);
static void _notify_text_insert_handler (BObject *obj,
  int position, int offset);
static void _notify_text_delete_handler (BObject *obj,
  int position, int offset);
static void _notify_caret_handler (BObject *obj, int position);
static void _notify_table_row_inserted (BObject *obj,
  gint start_offset, gint length);
static void _notify_table_column_inserted (BObject *obj,
  gint start_offset, gint length);
static void _notify_table_row_deleted (BObject *obj,
  gint start_offset, gint length);
static void _notify_table_column_deleted (BObject *obj,
  gint start_offset, gint length);
static void _notify_table_row_reordered (BObject *obj);
static void _notify_table_column_reordered (BObject *obj);
static void _notify_object_child_added (BObject *obj,
  gint index, BatkObject *child);
static void _notify_object_child_removed (BObject *obj,
  gint index, BatkObject *child);
static void _notify_object_state_change (BObject *obj,
  gchar *name, gboolean set);

/* Property handlers */

static void _property_change_handler (BatkObject *obj,
  BatkPropertyValues *values);

/* Ferret GUI callbacks */

void _action_cb(BtkWidget *widget, gpointer  *userdata);
void _toggle_terminal(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void _toggle_no_signals(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void _toggle_magnifier(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void _toggle_festival(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void _toggle_festival_terse(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void _toggle_trackmouse(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);
void _toggle_trackfocus(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data);

/* Global variables */
static BtkNotebook *notebook;
static TabInfo  *nbook_tabs[END_TABS];
static gint mouse_watcher_focus_id = -1;
static gint mouse_watcher_button_id = -1;
static gint focus_tracker_id = -1;
static gboolean use_magnifier = FALSE;
static gboolean use_festival = FALSE;
static gboolean track_mouse = FALSE;
static gboolean track_focus = TRUE;
static gboolean say_role = TRUE;
static gboolean say_accel = TRUE;
static gboolean display_ascii = FALSE;
static gboolean no_signals = FALSE;
static gint last_caret_offset = 0;

static BatkObject *last_object = NULL;
static BtkWidget *mainWindow = NULL;
static BtkWidget *vbox1 = NULL;
static BtkWidget *menu = NULL;
static BtkWidget *menutop = NULL;
static BtkWidget *menubar = NULL;
static BtkWidget *menuitem_terminal = NULL;
static BtkWidget *menuitem_no_signals = NULL;
static BtkWidget *menuitem_magnifier = NULL;
static BtkWidget *menuitem_festival = NULL;
static BtkWidget *menuitem_festival_terse = NULL;
static BtkWidget *menuitem_trackmouse = NULL;
static BtkWidget *menuitem_trackfocus = NULL;

#ifdef HAVE_SOCKADDR_UN_SUN_LEN
static struct sockaddr_un mag_server = { 0, AF_UNIX , "/tmp/magnifier_socket" };
static struct sockaddr_un client = { 0 , AF_UNIX, "/tmp/mag_client"};
#else
static struct sockaddr_un mag_server = { AF_UNIX , "/tmp/magnifier_socket" };
static struct sockaddr_un client = { AF_UNIX, "/tmp/mag_client"};
#endif

/* GUI Information for the output window */
typedef struct
{
  BtkWindow     *outputWindow;
  BtkWidget     *hbox;
  BtkWidget     *vbox;
  BtkWidget     *label;
  BtkWidget     *textInsert;
  gchar         *testTitle;
} MainDialog;

static void
_send_to_magnifier(gint x, gint y, gint w, gint h)
{
  int desc, length_msg;
  gchar buff[100];

  sprintf (buff, "~5:%d,%d", x+w/2, y+h/2);

#ifdef MAG_DEBUG
  g_print ("sending magnifier: %s\n", buff);
#endif

#ifdef HAVE_SOCKADDR_UN_SUN_LEN
  mag_server.sun_len = SUN_LEN(&mag_server);
  client.sun_len = SUN_LEN(&client);
#endif
  
  if((desc=socket(AF_UNIX,SOCK_STREAM,0)) == -1){
    perror("socket");
    return;
  }
  unlink("/tmp/mag_client");

  if (bind(desc, (struct sockaddr*)&client, sizeof(client)) == -1)
    {
      perror("bind");
      return;
    }

  if (connect(desc,(struct sockaddr*)&mag_server,sizeof(mag_server)) == -1)
    {
      perror("connect");
      return;
    }

  length_msg = write(desc,buff,strlen(buff));
  unlink("/tmp/mag_client");
  return;
}

static int _festival_init (void)
{
  int fd;
  struct sockaddr_in name;
  int tries = 2;

  name.sin_family = AF_INET;
  name.sin_port = htons (1314);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  fd = socket (PF_INET, SOCK_STREAM, 0);

  while (connect(fd, (struct sockaddr *) &name, sizeof (name)) < 0) {
    if (!tries--) {
      perror ("connect");
      return -1;
    }
  }

  _festival_write ("(audio_mode'async)", fd);
  return fd;
}

static void _festival_say (const gchar *text)
{
  static int fd = 0;
  gchar *quoted;
  gchar *p;
  gchar prefix [100];
  const gchar *stretch;

  fprintf (stderr, "saying %s\n", text);

  if (!fd)
    {
      fd = _festival_init ();
    }

  quoted = g_malloc(100+strlen(text)*2);

  stretch = g_strdup (g_getenv ("FESTIVAL_STRETCH"));
  if (!stretch) stretch = "0.75";
  sprintf (prefix, "(audio_mode'shutup)\n (Parameter.set 'Duration_Stretch %s)\n (SayText \"", stretch);
  
  strcpy(quoted, prefix);
  p = quoted + strlen (prefix);
  while (*text) {
    if ( *text == '\\' || *text == '"' )
      *p = '\\';
    *p++ = *text++;
  }
  *p++ = '"';
  *p++ = ')';
  *p = 0;

  _festival_write (quoted, fd);

  g_free(quoted);
}


static void _send_to_festival (const gchar *role_name,
  const gchar *name, const gchar *accel)
{
  gchar *string;
  int len = (strlen (role_name)+1 + strlen (name)+2 + 4 + strlen (accel)+2);
  int i, j;
  gchar ch;
  gchar *accel_name;
  
  string = (gchar *) g_malloc (len * sizeof (gchar));

  i = 0;
  if (say_role)
    {
      j = 0;
      while (role_name[j])
        {
          ch = role_name[j++];
          if (ch == '_') ch = ' ';
          string[i++] = ch;
        };
      string[i++] = ' ';
    }
  j = 0;
  while (name[j])
    {
      ch = name[j++];
      if (ch == '_') ch = ' ';
      string[i++] = ch;
    };
  if ((say_accel) && (strlen (accel) > 0))
    {
      accel_name = (gchar *)accel;
      if (strncmp (accel, "<C", 2) == 0)
        {
          accel_name = strncpy (accel_name, " control ", 9);
        }
      else if (strncmp (accel, " control", 5))
        {
          string[i++] = ' ';
          string[i++] = 'a';
          string[i++] = 'l';
          string[i++] = 't';
          string[i++] = ' ';
        }
      j = 0;
      while (accel_name[j])
        {
          ch = accel_name[j++];
          if (ch == '_') ch = ' ';
          string[i++] = ch;
        };
    }
  string[i] = '\0';

  _festival_say (string);
  g_free (string);
}

static void _festival_write (const gchar *command_string, int fd)
{
  gssize n_bytes;

  if (fd < 0) {
    perror("socket");
    return;
  }
  n_bytes = write(fd, command_string, strlen(command_string));
  g_assert (n_bytes == strlen(command_string));
}

static void _speak_caret_event (BatkObject *aobject)
{
  gint dummy1, dummy2;
  gint caret_offset = batk_text_get_caret_offset (BATK_TEXT (aobject));
  gchar * text;

  if (abs(caret_offset - last_caret_offset) > 1)
    {
      text = batk_text_get_text_at_offset (BATK_TEXT (aobject),
                                              caret_offset,
                                              BATK_TEXT_BOUNDARY_LINE_START,
                                              &dummy1,
                                              &dummy2);
    }
  else
    {
      text = batk_text_get_text_before_offset (BATK_TEXT (aobject),
                                              caret_offset,
                                              BATK_TEXT_BOUNDARY_CHAR,
                                              &dummy1,
                                              &dummy2);
    }
  _festival_say (text);
  g_free (text);
  last_caret_offset = caret_offset;
}

static void
_greyout_tab (BtkWidget *page_child, gboolean is_sensitive)
{
  BtkWidget *tab;

  tab = btk_notebook_get_tab_label (notebook, page_child);
  if (tab)
      btk_widget_set_sensitive (BTK_WIDGET (tab), is_sensitive);
}

static void
_refresh_notebook (BatkObject *aobject)
{
  if (BATK_IS_OBJECT (aobject))
  {
    _greyout_tab (nbook_tabs[ACTION]->page, BATK_IS_ACTION(aobject));
    _greyout_tab (nbook_tabs[COMPONENT]->page, BATK_IS_COMPONENT(aobject));
    _greyout_tab (nbook_tabs[IMAGE]->page, BATK_IS_IMAGE(aobject));
    _greyout_tab (nbook_tabs[SELECTION]->page, BATK_IS_SELECTION(aobject));
    _greyout_tab (nbook_tabs[TABLE]->page, BATK_IS_TABLE(aobject));
    _greyout_tab (nbook_tabs[TEXT]->page, BATK_IS_TEXT(aobject));
    _greyout_tab (nbook_tabs[VALUE]->page, BATK_IS_VALUE(aobject));
  }
}

static void _print_accessible (BatkObject *aobject)
{
  TabNumber top_tab;

  if (_object_is_ours(aobject))
    {
      if (display_ascii)
        g_print("\nFocus entered the ferret output window!\n");
      return;
    }

  _refresh_notebook(aobject);

  if (display_ascii)
    g_print("\nFocus change\n");

  /* Do not attach signal handlers if the user has asked not to */
  if (!no_signals)
    _update_handlers(aobject);
  else
    last_object = aobject; /* _update_handler normally does this */

  top_tab = btk_notebook_get_current_page (notebook) + OBJECT;
  _update(top_tab, aobject);

  if (use_magnifier)
    {
      gint x, y;
      gint w=0, h=0;
      
      if (BATK_IS_TEXT (aobject))
        {
	  gint x0, y0, w0, h0;
	  gint xN, yN, wN, hN;
	  gint len;
	  len = batk_text_get_character_count (BATK_TEXT (aobject));
	  batk_text_get_character_extents (BATK_TEXT (aobject), 0,
					  &x0, &y0, &w0, &h0,
					  BATK_XY_SCREEN);
          if (len > 0)
	    {
	      batk_text_get_character_extents (BATK_TEXT (aobject), len-1,
					      &xN, &yN, &wN, &hN,
					      BATK_XY_SCREEN);
	      x = MIN (x0, xN);
	      y = MIN (y0, yN);
	      w = MAX (x0+w0, xN+wN) - x;
	      h = MAX (y0+h0, yN+hN) - y;
	    }
          else
	    {
	      x = x0;
	      y = y0;
	    }
        } 
      else if (BATK_IS_COMPONENT (aobject))
        {
	  batk_component_get_extents (BATK_COMPONENT(aobject),
				     &x, &y, &w, &h,
				     BATK_XY_SCREEN);
        }
      if (w > -1) _send_to_magnifier (x, y, w, h);
    }
}

static gboolean
_object_is_ours (BatkObject *aobject)
{
  /* determine whether this object is parented by our own accessible... */

   BatkObject *toplevel = aobject;

   while (batk_object_get_role(aobject) != BATK_ROLE_FRAME)
     {
       aobject = batk_object_get_parent (aobject);
       if (aobject == NULL) break;
       toplevel = aobject;
     };

  /*
   * Some widgets do not have an BATK_ROLE_FRAME at the top,
   * so ignore those.
   */
   if (aobject != NULL)
     {
       if (BTK_ACCESSIBLE(toplevel)->widget == mainWindow)
         {
           return TRUE;
         }
     }

  return FALSE;
}

static gchar *
ferret_get_name_from_container (BatkObject *aobject)
{
  gchar *s = NULL;
  gint n = batk_object_get_n_accessible_children (aobject);
  gint i = 0;
  
  while (!s && (i < n))
    {
      BatkObject *child;	    
      child = batk_object_ref_accessible_child (aobject, i);
      if (BATK_IS_TEXT (child))
        {
		gint count = batk_text_get_character_count (BATK_TEXT (child));
		s = batk_text_get_text (BATK_TEXT (child),
				       0,
				       count);
        }
      g_object_unref (child);
      ++i;	    
    }
  
  if (!s)
    {
      s = g_strdup ("");
    }
  return s;	
}

static gint
_print_object (BatkObject *aobject)
{
    const gchar * parent_name = NULL;
    const gchar * name = NULL;
    const gchar * description = NULL;
    const gchar * typename = NULL;
    const gchar * parent_typename = NULL;
    const gchar * role_name = NULL;
    const gchar * accel_name = NULL;
    const gchar * text = NULL;
    BatkRole role;
    BatkObject *parent = NULL;
    static BatkObject *prev_aobject = NULL;
    gint n_children = 0;
    gint index_in_parent = -1;
    gchar *output_str;
    gint group_num;
    TabNumber tab_n = OBJECT;

    group_num = _print_groupname(tab_n, OBJECT_INTERFACE, "Object Interface");

    name = batk_object_get_name (aobject);
    typename = g_type_name (B_OBJECT_TYPE (aobject));
    description = batk_object_get_description (aobject);
    parent = batk_object_get_parent(aobject);
    if (parent)
      index_in_parent = batk_object_get_index_in_parent(aobject);
    n_children = batk_object_get_n_accessible_children(aobject);
    role = batk_object_get_role(aobject);
    role_name = batk_role_get_name(role);

    if (BATK_IS_ACTION (aobject))
      {
        accel_name = batk_action_get_keybinding (BATK_ACTION(aobject), 0);
        if (!accel_name) accel_name = "";
      }
    else
      {
        accel_name = "";
      }

    if (BTK_IS_ACCESSIBLE (aobject) &&
        BTK_IS_WIDGET (BTK_ACCESSIBLE (aobject)->widget))
      {
        _print_key_value(tab_n, group_num, "Widget name",
          (gpointer)btk_widget_get_name(BTK_ACCESSIBLE (aobject)->widget),
          VALUE_STRING);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Widget name", "No Widget",
          VALUE_STRING);
      }

    if (typename)
      {
        _print_key_value(tab_n, group_num, "Accessible Type",
          (gpointer)typename, VALUE_STRING);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Accessible Type", "NULL",
          VALUE_STRING);
      }

    if (name)
      {
        _print_key_value(tab_n, group_num, "Accessible Name",
          (gpointer)name, VALUE_STRING);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Accessible Name", "(unknown)",
          VALUE_STRING);
      }
    if (use_festival)
      {
        if (aobject != prev_aobject)
          {
	    if (BATK_IS_TEXT (aobject) && !name)
	      {
		text = 
		  batk_text_get_text_at_offset (BATK_TEXT (aobject),
					       (gint) 0,
					       BATK_TEXT_BOUNDARY_SENTENCE_END,
					       (gint *) NULL,
					       (gint *) NULL);
		fprintf (stderr, "first sentence: %s\n", text);
		_send_to_festival (role_name, 
				   text, "");
		if (!name) name = "no name";
	      }
            else 
	      { 
		text = "";
		if (!name)
		  {
		    if (batk_object_get_role (aobject) == BATK_ROLE_TABLE_CELL)
		      {
			gchar *cname = ferret_get_name_from_container (aobject);
			if (cname) name = g_strdup (cname);
		      }
		    else if (batk_object_get_role (aobject) == BATK_ROLE_CHECK_BOX)
		      {
			name = g_strdup ("check box");
		      }
		    else
		      {
		        name = "no name";
		      }
		  }
		_send_to_festival (role_name, name, accel_name);
	      }
          }
      }

    if (parent)
      {
        parent_name = batk_object_get_name(parent);

        parent_typename = g_type_name (B_OBJECT_TYPE (parent));

        if (parent_typename)
          {
            _print_key_value(tab_n, group_num, "Parent Accessible Type",
              (gpointer)parent_typename, VALUE_STRING);
          }
        else
          {
            _print_key_value(tab_n, group_num,
              "Parent Accessible Type", "NULL", VALUE_STRING);
          }

        if (parent_name)
          {
            _print_key_value(tab_n, group_num, "Parent Accessible Name",
              (gpointer)parent_name, VALUE_STRING);
          }
        else
          {
            _print_key_value(tab_n, group_num,
              "Parent Accessible Name", "NULL", VALUE_STRING);
          }

        output_str = g_strdup_printf("%d", index_in_parent);
        _print_key_value(tab_n, group_num, "Index in Parent",
          (gpointer)output_str, VALUE_STRING);
        g_free(output_str);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Parent", "NULL", VALUE_STRING);
      }

    if (description)
      {
        _print_key_value(tab_n, group_num, "Accessible Description",
          (gpointer)description, VALUE_STRING);
      }
    else
      {
        _print_key_value(tab_n, group_num,
          "Accessible Description", "NULL", VALUE_STRING);
      }

    if (role_name)
      {
      _print_key_value(tab_n, group_num, "Accessible Role", (gpointer)role_name,
        VALUE_STRING);
      }
    else
      {
      _print_key_value(tab_n, group_num, "Accessible Role", "NULL",
        VALUE_STRING);
      }

    output_str = g_strdup_printf("%d", n_children);
    _print_key_value(tab_n, group_num, "Number Children", (gpointer)output_str,
       VALUE_STRING);
    g_free(output_str);
    prev_aobject = aobject;

    return(group_num);
}

static gint
_print_relation (BatkObject *aobject)
{
    BatkRelationSet* relation_set = batk_object_ref_relation_set (aobject);
    gint n_relations =  batk_relation_set_get_n_relations (relation_set);
    gint group_num;
    TabNumber tab_n = OBJECT;

    group_num = _print_groupname(tab_n, RELATION_INTERFACE,
      "Relation Interface");

    if (relation_set)
      {
        BatkRelation * relation;
        const gchar * relation_name = NULL;
        const gchar * relation_obj_name = NULL;
        BatkRelationType relation_type;
        BatkObject *relation_obj;
        GPtrArray * relation_arry;
        gchar *label_str;
        gchar *output_str;
        gint i, j;

        output_str = g_strdup_printf("%d", n_relations);
        _print_key_value(tab_n, group_num,
          "Number of Relations", (gpointer)output_str, VALUE_STRING);
        g_free(output_str);

        for (i = 0; i < n_relations; i++)
          {
            relation = batk_relation_set_get_relation (relation_set, i);

            relation_type = batk_relation_get_relation_type (relation);
            relation_name = batk_relation_type_get_name (relation_type);

            relation_arry = batk_relation_get_target(relation);

            if (relation_name)
              {
                label_str = g_strdup_printf("Relation %d Name", i + 1);
                _print_key_value(tab_n, group_num, label_str,
                  (gpointer)relation_name, VALUE_STRING);
                g_free(label_str);
              }
            else
              {
                label_str = g_strdup_printf("Relation %d Type", i + 1);
                output_str = g_strdup_printf("%d", relation_type);
                _print_key_value(tab_n, group_num, label_str,
                  (gpointer)output_str, VALUE_STRING);
                g_free(label_str);
                g_free(output_str);
              }

            label_str = g_strdup_printf("Relation %d with", i + 1);
            output_str = g_strdup_printf("%d BatkObjects", relation_arry->len);
            _print_key_value(tab_n, group_num, label_str, (gpointer)output_str,
              VALUE_STRING);
            g_free(label_str);
            g_free(output_str);

            for (j=0; j < relation_arry->len; j++)
              {
                label_str = g_strdup_printf(
                  "Relation %d,%d with BatkObject Name", i + 1, j + 1);
                relation_obj = (BatkObject *)
                   g_ptr_array_index(relation_arry, j);
                relation_obj_name = batk_object_get_name(relation_obj);

                _print_key_value(tab_n, group_num, label_str,
                  (gpointer)relation_obj_name, VALUE_STRING);
                g_free(label_str);
              }
          }

        g_object_unref (relation_set);
      }
    return(group_num);
}

static gint
_print_state (BatkObject *aobject)
{
    BatkStateSet *state_set = batk_object_ref_state_set(aobject);
    gint group_num;
    TabNumber tab_n = OBJECT;
    static BatkStateType states_to_track[] =
      {
        BATK_STATE_ACTIVE,
        BATK_STATE_CHECKED,
        BATK_STATE_EXPANDED,
        BATK_STATE_EXPANDABLE,
        BATK_STATE_SELECTED,
        BATK_STATE_SHOWING,
        BATK_STATE_VISIBLE
      };

    group_num = _print_groupname(tab_n, STATE_INTERFACE,
      "State Interface");

    if (state_set)
      {
        gboolean boolean_value;
        BatkStateType one_state;
        const gchar *name;
        gint i;

        for (i=0; i < sizeof(states_to_track)/sizeof(BatkStateType); i++)
          {
            one_state = (BatkStateType) states_to_track[i];
            name = batk_state_type_get_name (one_state);

            if (name)
              {
                boolean_value =
                  batk_state_set_contains_state (state_set, one_state);
                _print_key_value(tab_n, group_num, name,
                  (gpointer)(&boolean_value), VALUE_BOOLEAN);
              }
          }
      }

    g_object_unref (state_set);
    return(group_num);
}

static gint
_print_action (BatkAction *aobject)
{
    const gchar *action_name;
    const gchar *action_description;
    const gchar *action_keybinding;
    gchar *label_str, *output_str;
    gint group_num;
    gint num_actions, j;
    TabNumber tab_n = ACTION;
    NameValue *nv;

    group_num = _print_groupname(tab_n, ACTION_INTERFACE,
      "Action Interface");

    num_actions = batk_action_get_n_actions (aobject);
    output_str = g_strdup_printf("%d", num_actions);
    _print_key_value(tab_n, group_num, "Number of Actions",
      (gpointer) output_str, VALUE_STRING);
    g_free(output_str);

    for (j = 0; j < num_actions; j++)
      {
        label_str = g_strdup_printf("Action %d Name", j + 1);
        action_name = batk_action_get_name (aobject, j);
        if (action_name)
          {
            nv = _print_key_value(tab_n, group_num, label_str,
             (gpointer) action_name, VALUE_BUTTON);
          }
        else
          {
            nv = _print_key_value(tab_n, group_num, label_str, "NULL",
              VALUE_BUTTON);
          }

        nv->batkobj = BATK_OBJECT(aobject);
        nv->action_num = j;
        nv->signal_id = g_signal_connect (BTK_OBJECT (nv->button),
          "clicked", G_CALLBACK (_action_cb), nv);

        g_free(label_str);

        label_str = g_strdup_printf("Action %d Description", j + 1);
        action_description = batk_action_get_description (aobject, j);
        if (action_description)
          {
            _print_key_value(tab_n, group_num, label_str,
              (gpointer)action_description, VALUE_STRING);
          }
        else
          {
            _print_key_value(tab_n, group_num, label_str, "NULL",
              VALUE_STRING);
          }
        g_free(label_str);

        label_str = g_strdup_printf("Action %d Keybinding", j + 1);
        action_keybinding = batk_action_get_keybinding (aobject, j);
        if (action_keybinding)
          {
            _print_key_value(tab_n, group_num, label_str,
              (gpointer)action_keybinding, VALUE_STRING);
          }
        else
          {
            _print_key_value(tab_n, group_num, label_str, "NULL",
              VALUE_STRING);
          }
        g_free(label_str);
      }
    return(group_num);
}

static gint
_print_component (BatkComponent *aobject)
{
    gchar *output_str;
    gint x = 0;
    gint y = 0;
    gint width = 0;
    gint height = 0;
    gint group_num;
    TabNumber tab_n = COMPONENT;

    group_num = _print_groupname(tab_n, COMPONENT_INTERFACE,
      "Component Interface");

    batk_component_get_extents (aobject,
       &x, &y, &width, &height, BATK_XY_SCREEN);

    output_str = g_strdup_printf("x: %d y: %d width: %d height %d",
      x, y, width, height);
    _print_key_value(tab_n, group_num, "Geometry", (gpointer)output_str,
      VALUE_STRING);
    g_free(output_str);
    return(group_num);
}

static gint
_print_image (BatkImage *aobject)
{
    const gchar *image_desc;
    gchar *output_str;
    gint x = 0;
    gint y = 0;
    gint height = 0;
    gint width = 0;
    gint group_num;
    TabNumber tab_n = IMAGE;

    group_num = _print_groupname(tab_n, IMAGE_INTERFACE,
      "Image Interface");

    image_desc = batk_image_get_image_description(aobject);
    if (image_desc)
      {
        _print_key_value(tab_n, group_num, "Description", (gpointer)image_desc,
          VALUE_STRING);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Description", "NULL",
          VALUE_STRING);
      }

    batk_image_get_image_position(aobject, &x, &y, BATK_XY_SCREEN);
    batk_image_get_image_size(aobject, &height, &width);

    output_str = g_strdup_printf("x: %d y: %d width: %d height %d",
       x, y, width, height);
    _print_key_value(tab_n, group_num, "Geometry", (gpointer)output_str,
      VALUE_STRING);
    g_free(output_str);
    return(group_num);
}

static gint
_print_selection (BatkSelection *aobject)
{
    BatkObject *object;
    BatkRole role;
    gchar *label_str, *output_str;
    gint group_num;
    gint n_selected, j, n_selectable;
    TabNumber tab_n = SELECTION;

    group_num = _print_groupname(tab_n, SELECTION_INTERFACE,
      "Selection Interface");

    n_selected = batk_selection_get_selection_count (aobject);
    output_str = g_strdup_printf ("%d", n_selected);
    _print_key_value (tab_n, group_num, "Number of Selected Children",
                      (gpointer) output_str, VALUE_STRING);
    g_free (output_str);
    /*
     * The number of selected items is the number of children except for
     * a ComboBox where it is the number of items in the list.
     */
    object = BATK_OBJECT (aobject);
    role = batk_object_get_role (object);
    if (role == BATK_ROLE_COMBO_BOX)
    {
      object = batk_object_ref_accessible_child (object, 0);
      g_return_val_if_fail (batk_object_get_role (object) == BATK_ROLE_LIST,
                            group_num);
      n_selectable = batk_object_get_n_accessible_children (object);
      g_object_unref (B_OBJECT (object)); 
    }
    else
    {
      n_selectable = batk_object_get_n_accessible_children (object);
    }
    output_str = g_strdup_printf ("%d", n_selectable);
    _print_key_value (tab_n, group_num, "Number of Selectable Children",
                      (gpointer) output_str, VALUE_STRING);
    g_free (output_str);

    for (j = 0; j < n_selected; j++)
    {
      const gchar *selected_name;
      BatkObject *selected_object;

      selected_object = batk_selection_ref_selection (aobject, j);
      selected_name = batk_object_get_name (selected_object);
      if (selected_name == NULL)
      {
        selected_name = "No name";
      }
      label_str = g_strdup_printf ("Selected item: %d Name", j+1);
      _print_key_value (tab_n, group_num, label_str,
                        (gpointer) selected_name, VALUE_STRING);
      g_free (label_str);
      g_object_unref (B_OBJECT (selected_object));
    }
    return group_num;
}

static gint
_print_table (BatkTable *aobject)
{
    gchar *label_str, *output_str;
    const gchar *col_desc;
    BatkObject *caption;
    gint n_cols, n_rows;
    gint i;
    gint group_num;
    TabNumber tab_n = TABLE;

    group_num = _print_groupname(tab_n, TABLE_INTERFACE,
      "Table Interface");

    n_cols = batk_table_get_n_columns(aobject);
    output_str = g_strdup_printf("%d", n_cols);
    _print_key_value(tab_n, group_num, "Number Columns", (gpointer)output_str,
      VALUE_STRING);
    g_free(output_str);

    n_rows = batk_table_get_n_rows(aobject);
    output_str = g_strdup_printf("%d", n_rows);
    _print_key_value(tab_n, group_num, "Number Rows", (gpointer)output_str,
      VALUE_STRING);
    g_free(output_str);

    caption = batk_table_get_caption(aobject);
    if (caption)
      {
        const gchar* caption_name;

        caption_name = batk_object_get_name (caption);
        if (caption_name)
          {
            _print_key_value(tab_n, group_num, "Caption Name", 
                             (gpointer)caption_name, VALUE_STRING);
          }
      }

    for (i=0; i < n_cols; i++)
      {
        label_str = g_strdup_printf("Column %d Description", i + 1);

        col_desc = batk_table_get_column_description(aobject, i);
        if (col_desc)
          {
            _print_key_value(tab_n, group_num, label_str, (gpointer)col_desc,
              VALUE_STRING);
          }
        else
          {
            _print_key_value(tab_n, group_num, label_str, "NULL",
              VALUE_STRING);
          }

        g_free(label_str);
      }

    return(group_num);
}

static gint
_print_text (BatkText *aobject)
{
    gchar *output_str, *text_val, *text_val_escaped;
    gint n_chars, caret_offset;
    gint start_offset, end_offset;
    gint group_num;
    gint x, y, w, h;
    TabNumber tab_n = TEXT;

    group_num = _print_groupname(tab_n, TEXT_INTERFACE,
      "Text Content");

    n_chars = batk_text_get_character_count(aobject);

    output_str = g_strdup_printf("%d", n_chars);
    _print_key_value(tab_n, group_num, "Total Character Count",
      (gpointer)output_str, VALUE_STRING);
    g_free(output_str);

   /*
    * Pass through g_strescape so that non-ASCII chars are made
    * print-able.
    */
    text_val = batk_text_get_text (aobject, 0, n_chars);
    if (text_val)
      {
        text_val_escaped = g_strescape(text_val, NULL);
        _print_key_value (tab_n, group_num, "Text", (gpointer)text_val_escaped,
          VALUE_TEXT);
        g_free (text_val);
        g_free (text_val_escaped);
      }
    else
      {
        _print_key_value (tab_n, group_num, "Text", "NULL", VALUE_TEXT);
      }

    caret_offset = batk_text_get_caret_offset(aobject);
    output_str = g_strdup_printf("%d", caret_offset);
    _print_key_value(tab_n, group_num, "Caret Offset", (gpointer)output_str,
      VALUE_STRING);
    g_free(output_str);

    if (caret_offset < 0)
      return(group_num);

    text_val = batk_text_get_text_at_offset (aobject, caret_offset,
                                            BATK_TEXT_BOUNDARY_CHAR,
                                            &start_offset, &end_offset);
    if (text_val)
      {
        text_val_escaped = g_strescape(text_val, NULL);
        _print_key_value(tab_n, group_num, "Current Character",
          (gpointer)text_val_escaped, VALUE_STRING);
        g_free (text_val);
        g_free (text_val_escaped);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Current Character", "none",
          VALUE_STRING);
      }

    batk_text_get_character_extents (aobject, caret_offset,
                                    &x, &y, &w, &h, BATK_XY_SCREEN);
    output_str = g_strdup_printf ("(%d, %d) (%d, %d)", x, y, w, h);
    if (output_str)
      {
        _print_key_value(tab_n, group_num, "Character Bounds (screen)",
          (gpointer)output_str, VALUE_STRING);
        g_free(output_str);
      }

    batk_text_get_character_extents (aobject, caret_offset,
                                    &x, &y, &w, &h, BATK_XY_WINDOW);
    output_str = g_strdup_printf ("(%d, %d) (%d, %d)", x, y, w, h);
    if (output_str)
      {
        _print_key_value(tab_n, group_num, "Character Bounds (window)",
          (gpointer)output_str, VALUE_STRING);
        g_free(output_str);
      }

    text_val = batk_text_get_text_at_offset (aobject, caret_offset,
                                            BATK_TEXT_BOUNDARY_WORD_START,
                                            &start_offset, &end_offset);
    if (text_val)
      {
        text_val_escaped = g_strescape(text_val, NULL);
        _print_key_value(tab_n, group_num, "Current Word",
          (gpointer)text_val_escaped, VALUE_STRING);
        g_free (text_val);
        g_free (text_val_escaped);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Current Word", "none",
          VALUE_STRING);
      }

    text_val = batk_text_get_text_at_offset (aobject, caret_offset,
                                            BATK_TEXT_BOUNDARY_LINE_START,
                                            &start_offset, &end_offset);
    if (text_val)
      {
        text_val_escaped = g_strescape(text_val, NULL);
        _print_key_value(tab_n, group_num, "Current Line",
          (gpointer)text_val_escaped, VALUE_STRING);
        g_free (text_val);
        g_free (text_val_escaped);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Current Line", "none",
          VALUE_STRING);
      }

    text_val = batk_text_get_text_at_offset (aobject, caret_offset,
                                              BATK_TEXT_BOUNDARY_SENTENCE_START,
                                              &start_offset, &end_offset);
    if (text_val)
      {
        text_val_escaped = g_strescape(text_val, NULL);
        _print_key_value(tab_n, group_num, "Current Sentence",
          (gpointer)text_val_escaped, VALUE_STRING);
        g_free (text_val);
        g_free (text_val_escaped);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Current Line", "none",
          VALUE_STRING);
      }
    return(group_num);
}

static gint
_print_text_attributes (BatkText *aobject)
{
    BatkAttributeSet *attribute_set;
    BatkAttribute *attribute;
    gchar *output_str, *label_str;
    gint start_offset, end_offset, caret_offset;
    gint attribute_set_len, attribute_offset, i;
    gint n_chars;
    gint group_num;
    TabNumber tab_n = TEXT;

    n_chars = batk_text_get_character_count(aobject);

    group_num = _print_groupname (tab_n, TEXT_ATTRIBUTES,
      "Text Attributes at Caret");

    caret_offset = batk_text_get_caret_offset(aobject);
    attribute_offset = caret_offset;

    start_offset = 0;
    end_offset = 0;

    attribute_set = batk_text_get_run_attributes(aobject, attribute_offset,
          &start_offset, &end_offset);

    label_str = g_strdup_printf("Attribute run start");

    output_str = g_strdup_printf("%d", start_offset);
    _print_key_value(tab_n, group_num, label_str, (gpointer)output_str,
                     VALUE_STRING);
    g_free(label_str);
    g_free(output_str);

    label_str = g_strdup_printf("Attribute run end");
    output_str = g_strdup_printf("%d", end_offset);
    _print_key_value(tab_n, group_num, label_str, (gpointer)output_str,
                     VALUE_STRING);
    g_free(label_str);
    g_free(output_str);

    if (attribute_set == NULL)
      attribute_set_len = 0;
    else
      attribute_set_len = b_slist_length(attribute_set);

    label_str = g_strdup_printf("Number of Attributes");
    output_str = g_strdup_printf("%d", attribute_set_len);
    _print_key_value(tab_n, group_num, label_str, (gpointer)output_str,
                     VALUE_STRING);
    g_free(label_str);
    g_free(output_str);

    for (i=0; i < attribute_set_len; i++)
      {
        attribute = ((BatkAttribute *) b_slist_nth(attribute_set, i)->data);

        _print_key_value(tab_n, group_num, attribute->name,
                         (gpointer)attribute->value, VALUE_STRING);
      }
    if (attribute_set != NULL)
      batk_attribute_set_free(attribute_set);


    return(group_num);
}

static gint
_print_value (BatkValue *aobject)
{
    BValue *value_back, val;
    gint group_num;
    TabNumber tab_n = VALUE;

    value_back = &val;

    group_num = _print_groupname(tab_n, VALUE_INTERFACE,
      "Value Interface");

    batk_value_get_current_value(aobject, value_back);
    _print_value_type(group_num, "Value", value_back);
    batk_value_get_minimum_value(aobject, value_back);
    _print_value_type(group_num, "Minimum Value", value_back);
    batk_value_get_maximum_value(aobject, value_back);
    _print_value_type(group_num, "Maximum Value", value_back);
    return(group_num);
}

static void
_print_value_type(gint group_num, gchar *type, BValue *value)
{
    gchar *label_str = NULL;
    gchar *output_str = NULL;
    TabNumber tab_n = VALUE;

    if (G_VALUE_HOLDS_DOUBLE (value))
      {
        label_str = g_strdup_printf("%s - Double", type);
        output_str = g_strdup_printf("%f",
          b_value_get_double (value));
        _print_key_value(tab_n, group_num, label_str, (gpointer)output_str,
          VALUE_STRING);
      }
    else if (G_VALUE_HOLDS_INT (value))
      {
        label_str = g_strdup_printf("%s - Integer", type);
        output_str = g_strdup_printf("%d",
          b_value_get_int (value));
        _print_key_value(tab_n, group_num, label_str, (gpointer)output_str,
          VALUE_STRING);
      }
    else
      {
        _print_key_value(tab_n, group_num, "Value", "Unknown Type",
          VALUE_STRING);
      }

    if (label_str)
        g_free(label_str);
    if (output_str)
        g_free(output_str);
}

static void
_create_event_watcher (void)
{
    focus_tracker_id = batk_add_focus_tracker (_print_accessible);

    if (track_mouse)
      {
        mouse_watcher_focus_id =
          batk_add_global_event_listener(_mouse_watcher,
          "Btk:BtkWidget:enter_notify_event");
        mouse_watcher_button_id =
          batk_add_global_event_listener(_button_watcher,
          "Btk:BtkWidget:button_press_event");
      }
}

static gboolean
_mouse_watcher (GSignalInvocationHint *ihint,
               guint                  n_param_values,
               const BValue          *param_values,
               gpointer               data)
{
    BObject *object;
    BtkWidget *widget;

    object = b_value_get_object (param_values + 0);

    if (BTK_IS_MENU(object)) return TRUE;

    g_assert (BTK_IS_WIDGET(object));

    widget = BTK_WIDGET (object);
    if (BTK_IS_WINDOW (widget))
    {
        BtkWidget *focus_widget = BTK_WINDOW (widget)->focus_widget;
        if (focus_widget != NULL)
            widget = focus_widget;
    }

    _print_accessible (btk_widget_get_accessible (widget));
    return TRUE;
}

static gboolean
_button_watcher (GSignalInvocationHint *ihint,
                 guint                  n_param_values,
                 const BValue          *param_values,
                 gpointer               data)
{
    BObject *object;
    BtkWidget *widget;

    object = b_value_get_object (param_values + 0);

    widget = BTK_WIDGET (object);
    if (BTK_IS_CONTAINER (widget))
    {
      if (G_VALUE_HOLDS_BOXED (param_values + 1))
        {
          BdkEventButton *event;
          gpointer gp;
          BatkObject *aobject;
          BatkObject *child;
          gint  aobject_x, aobject_y;
          gint x, y;

          gp = b_value_get_boxed (param_values + 1);
          event = (BdkEventButton *) gp;
          aobject = btk_widget_get_accessible (widget);
          aobject_x = aobject_y = 0;
          batk_component_get_position (BATK_COMPONENT (aobject), 
                                      &aobject_x, &aobject_y, 
                                      BATK_XY_WINDOW);
          x = aobject_x + (gint) event->x; 
          y = aobject_y + (gint) event->y; 
          child = batk_component_ref_accessible_at_point (BATK_COMPONENT (aobject),
                                                         x,
                                                         y,
                                                         BATK_XY_WINDOW);
          if (child)
            {
              _print_accessible (child);
              g_object_unref (child);
            }
          else
            {
              if (!BTK_IS_MENU_ITEM (widget))
                {
                  g_print ("No child at position %d %d for %s\n", 
                           x,
                           y,
                           g_type_name (B_OBJECT_TYPE (widget)));
                }
            }
        }
    }

    return TRUE;
}

static void _add_notebook_page (BtkNotebook *nbook,
                                BtkWidget *content_widget,
                                BtkWidget **new_page,
                                const gchar *label_text)
{
  BtkWidget *label;

  if (content_widget != NULL)
    {
      *new_page = content_widget;
    }
  else
    {
      *new_page = btk_vpaned_new ();
    }

  label = btk_label_new ("");
  btk_label_set_markup_with_mnemonic (BTK_LABEL (label), label_text);
  btk_notebook_append_page (notebook, *new_page, label);
  btk_widget_show(*new_page);
}

static void
_create_notebook (void)
{
  TabInfo *tab;
  notebook = BTK_NOTEBOOK (btk_notebook_new());

  tab = nbook_tabs[OBJECT];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Object</b>");

  tab = nbook_tabs[ACTION];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Action</b>");

  tab = nbook_tabs[COMPONENT];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Component</b>");

  tab = nbook_tabs[IMAGE];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Image</b>");

  tab = nbook_tabs[SELECTION];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Selection</b>");

  tab = nbook_tabs[TABLE];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Table</b>");

  tab = nbook_tabs[TEXT];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>Te_xt</b>");

  tab = nbook_tabs[VALUE];
  _add_notebook_page (notebook, tab->main_box, &tab->page, "<b>_Value</b>");

  g_signal_connect (BTK_OBJECT (notebook),
                      "switch-page",
                      G_CALLBACK (_update_current_page),
                      NULL);
}

static void
_init_data(void)
{
  TabInfo *the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Object";
  nbook_tabs[OBJECT] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Action";
  nbook_tabs[ACTION] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Component";
  nbook_tabs[COMPONENT] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Image";
  nbook_tabs[IMAGE] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Selection";
  nbook_tabs[SELECTION] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Table";
  nbook_tabs[TABLE] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Text";
  nbook_tabs[TEXT] = the_tab;

  the_tab = g_new0(TabInfo, 1);
  the_tab->page = NULL;
  the_tab->main_box = btk_vbox_new(FALSE, 20);
  the_tab->name = "Value";
  nbook_tabs[VALUE] = the_tab;
}

static void
_create_window (void)
{
    static BtkWidget *window = NULL;

    if (!window)
    {
        window = btk_window_new (BTK_WINDOW_TOPLEVEL);
        btk_widget_set_name (window, "Ferret Window");
        btk_window_set_policy (BTK_WINDOW(window), TRUE, TRUE, FALSE);

        g_signal_connect (BTK_OBJECT (window), "destroy",
                           G_CALLBACK (btk_widget_destroyed),
                           &window);

        btk_window_set_title (BTK_WINDOW (window), "BTK+ Ferret Output");
        btk_window_set_default_size (BTK_WINDOW (window), 333, 550);
        btk_container_set_border_width (BTK_CONTAINER (window), 0);

        vbox1 = btk_vbox_new (FALSE, 0);
        btk_container_add (BTK_CONTAINER (window), vbox1);
        btk_widget_show (vbox1);

        menubar = btk_menu_bar_new ();
        btk_box_pack_start (BTK_BOX (vbox1), menubar, FALSE, TRUE, 0);
        btk_widget_show (menubar);
        menutop = btk_menu_item_new_with_label("Menu");
        btk_menu_bar_append(BTK_MENU_BAR(menubar), menutop);
        btk_widget_show (menutop);
        menu = btk_menu_new();
        btk_menu_item_set_submenu (BTK_MENU_ITEM (menutop), menu);
        btk_widget_show (menu);

        _add_menu(&menu, &menuitem_trackmouse, "Track Mouse", track_mouse,
           G_CALLBACK(_toggle_trackmouse));
        _add_menu(&menu, &menuitem_trackfocus, "Track Focus", track_focus,
           G_CALLBACK(_toggle_trackfocus));
        _add_menu(&menu, &menuitem_magnifier, "Magnifier", use_magnifier,
           G_CALLBACK(_toggle_magnifier));
        _add_menu(&menu, &menuitem_festival, "Festival", use_festival,
           G_CALLBACK(_toggle_festival));
        _add_menu(&menu, &menuitem_festival_terse, "Festival Terse",
          (!say_role && !say_accel),
          G_CALLBACK(_toggle_festival_terse));
        _add_menu(&menu, &menuitem_terminal, "Terminal Output", display_ascii,
           G_CALLBACK(_toggle_terminal));
        _add_menu(&menu, &menuitem_no_signals, "No BATK Signals", no_signals,
           G_CALLBACK(_toggle_no_signals));

        _create_notebook ();
        btk_container_add (BTK_CONTAINER (vbox1), BTK_WIDGET (notebook));
        btk_widget_show (BTK_WIDGET (notebook));
    }
    if (!btk_widget_get_visible (window))
        btk_widget_show (window);

    mainWindow = BTK_WIDGET (window);
}

static void
_add_menu(BtkWidget ** menu, BtkWidget ** menuitem, gchar * name,
  gboolean init_value, GCallback func)
{
    *menuitem = btk_check_menu_item_new_with_label(name);
    btk_check_menu_item_set_active(
      BTK_CHECK_MENU_ITEM(*menuitem), init_value);
    btk_menu_shell_append (BTK_MENU_SHELL (*menu), *menuitem);
    btk_widget_show(*menuitem);
    g_signal_connect(BTK_OBJECT(*menuitem), "toggled", func, NULL);
}

int
btk_module_init(gint argc, char* argv[])
{
    if (g_getenv ("FERRET_ASCII"))
       display_ascii = TRUE;

    if (g_getenv ("FERRET_NOSIGNALS"))
       no_signals = TRUE;

    if (display_ascii)
       g_print("BTK ferret Module loaded\n");

    if (g_getenv("FERRET_MAGNIFIER"))
        use_magnifier = TRUE;

    if (g_getenv ("FERRET_FESTIVAL"))
        use_festival = TRUE;

    if (g_getenv ("FERRET_MOUSETRACK"))
        track_mouse = TRUE;

    if (g_getenv ("FERRET_TERSE"))
      {
        say_role = FALSE;
        say_accel = FALSE;
      }

    _init_data();

    _create_window();

    _create_event_watcher();

    return 0;
}

static void
_clear_tab(TabNumber tab_n)
{
    GList *group_list, *nv_list;
    TabInfo *tab;
    GroupInfo *group;
    NameValue *nv;

    tab = nbook_tabs[tab_n];

    for (group_list = tab->groups; group_list; group_list = group_list->next)
      {
        group = (GroupInfo *) group_list->data;

        if (group->is_scrolled)
          btk_widget_hide(BTK_WIDGET(group->scroll_outer_frame));

        btk_widget_hide(BTK_WIDGET(group->frame));
        btk_widget_hide(BTK_WIDGET(group->group_vbox));

        for (nv_list = group->name_value; nv_list; nv_list = nv_list->next)
          {
            nv = (NameValue *) nv_list->data;
            nv->active = FALSE;
            btk_widget_hide(BTK_WIDGET(nv->column1));
            btk_widget_hide(BTK_WIDGET(nv->column2));
            btk_widget_hide(BTK_WIDGET(nv->label));

            switch (nv->type)
              {
              case VALUE_STRING:
                btk_widget_hide(BTK_WIDGET(nv->string));
                break;
              case VALUE_BOOLEAN:
                btk_widget_hide(BTK_WIDGET(nv->boolean));
                break;
              case VALUE_TEXT:
                btk_widget_hide(BTK_WIDGET(nv->text));
                break;
              case VALUE_BUTTON:
                btk_widget_hide(BTK_WIDGET(nv->button));
                break;
              }
            btk_widget_hide(BTK_WIDGET(nv->hbox));

            /* Disconnect signal handler if any */
            if (nv->signal_id != -1)
              g_signal_handler_disconnect(nv->button, nv->signal_id);

            nv->signal_id = -1;
          }
      }
}

static gint
_print_groupname(TabNumber tab_n, GroupId group_id,
  const char *groupname)
{
  TabInfo *tab;
  GroupInfo *the_group;
  gint rc = -1;

  if (display_ascii)
      g_print("\n<%s>\n", groupname);

  tab = nbook_tabs[tab_n];
  the_group = _get_group(tab, group_id, groupname);
  rc = g_list_index(tab->groups, the_group);
  return rc;
}

static GroupInfo*
_get_group(TabInfo *tab, GroupId group_id, const gchar *groupname)
{
    GroupInfo *group = NULL;
    gboolean found = FALSE;
    GList *group_list;

    for (group_list = tab->groups; group_list; group_list = group_list->next)
      {
        group = (GroupInfo *) group_list->data;
        if (group_id == group->group_id)
          {
            found = TRUE;
            break;
          }
      }

   if (!found)
     {
       /* build a new one */
       group = (GroupInfo *)g_new0(GroupInfo, 1);
       group->group_id = group_id;
       _get_group_scrolled(group);

       if (group->is_scrolled)
         {
           group->frame = btk_scrolled_window_new (NULL, NULL);
           btk_widget_set_size_request (BTK_WIDGET (group->frame), -2,
             group->default_height);
           group->scroll_outer_frame = BTK_FRAME(btk_frame_new(groupname));
           btk_container_add(BTK_CONTAINER(group->scroll_outer_frame),
             group->frame);
         }
       else
         {
           group->frame = btk_frame_new(groupname);
         }

       btk_container_set_border_width(BTK_CONTAINER(group->frame), 10);

       group->name = g_strdup(groupname);
       group->group_vbox = BTK_VBOX(btk_vbox_new(FALSE, 10));

       if (group->is_scrolled)
         {
           btk_scrolled_window_set_policy (BTK_SCROLLED_WINDOW (group->frame),
              BTK_POLICY_AUTOMATIC, BTK_POLICY_AUTOMATIC);
           btk_scrolled_window_add_with_viewport(
             BTK_SCROLLED_WINDOW(group->frame),
             BTK_WIDGET(group->group_vbox));
         }
       else
         {
           btk_container_add(BTK_CONTAINER(group->frame),
             BTK_WIDGET(group->group_vbox));
         }

       tab->groups = g_list_append (tab->groups, group);

       if (group->is_scrolled)
         {
           btk_box_pack_start (BTK_BOX (tab->main_box),
                               BTK_WIDGET (group->scroll_outer_frame),
                               TRUE, TRUE, 0);
         }
       else
         {
           btk_box_pack_start (BTK_BOX (tab->main_box),
                               BTK_WIDGET (group->frame),
                               TRUE, TRUE, 0);
         }
     }

   return group;
}

void
_get_group_scrolled(GroupInfo *group)
{
   if (group->group_id == OBJECT_INTERFACE)
     {
       group->is_scrolled = FALSE;
     }
   else if (group->group_id == RELATION_INTERFACE)
     {
       group->is_scrolled = TRUE;
       group->default_height = 50;
     }
   else if (group->group_id == STATE_INTERFACE)
     {
       group->is_scrolled = TRUE;
       group->default_height = 100;
     }
   else if (group->group_id == ACTION_INTERFACE)
     {
       group->is_scrolled = TRUE;
       group->default_height = 200;
     }
   else if (group->group_id == TEXT_ATTRIBUTES)
     {
       group->is_scrolled = TRUE;
       group->default_height = 70;
     }
   else
     {
       group->is_scrolled = FALSE;
     }
}

NameValue *
_get_name_value(GroupInfo *group, const gchar *label,
  gpointer value_ptr, ValueType type)
{
    NameValue *name_value = NULL;
    GList *nv_list;
    BValue *value;
    gboolean found = FALSE;
    static char *empty_string = "";

    if (!label)
      {
        label = empty_string;
      }

    for (nv_list = group->name_value; nv_list; nv_list = nv_list->next)
      {
        name_value = (NameValue *) nv_list->data;
        if (!name_value->active)
          {
            found = TRUE;
            break;
          }
      }

    if (!found)
      {
        name_value = (NameValue *)g_new0(NameValue, 1);
        name_value->column1 = BTK_HBOX(btk_hbox_new(FALSE, 10));
        name_value->column2 = BTK_HBOX(btk_hbox_new(FALSE, 10));
        name_value->hbox = BTK_HBOX(btk_hbox_new(FALSE, 5));
        name_value->label = BTK_LABEL(btk_label_new(label));
        name_value->string = btk_label_new (NULL);
        name_value->boolean = btk_check_button_new ();
        name_value->text = btk_entry_new_with_max_length (1000);
        name_value->button = BTK_BUTTON(btk_button_new ());

        btk_box_pack_end(BTK_BOX(name_value->column1),
          BTK_WIDGET(name_value->label), FALSE, FALSE, 10);

        switch (type)
          {
          case VALUE_STRING:
            btk_label_set_text(BTK_LABEL(name_value->string),
              (gchar *) value_ptr);
            btk_box_pack_start(BTK_BOX(name_value->column2),
              BTK_WIDGET(name_value->string), FALSE, FALSE, 10);
            break;
          case VALUE_BOOLEAN:
            btk_toggle_button_set_active(BTK_TOGGLE_BUTTON(name_value->boolean),
               *((gboolean *)value_ptr));
            btk_widget_set_sensitive(name_value->boolean, FALSE);
            btk_box_pack_start(BTK_BOX(name_value->column2),
              BTK_WIDGET(name_value->boolean), FALSE, FALSE, 10);
            break;
          case VALUE_TEXT:
            btk_entry_set_text (BTK_ENTRY (name_value->text),
              (gchar *)value_ptr);
            btk_box_pack_start(BTK_BOX(name_value->column2),
              BTK_WIDGET(name_value->text), FALSE, FALSE, 10);
          case VALUE_BUTTON:
            value = &(name_value->button_gval);
            memset (value, 0, sizeof (BValue));
            b_value_init (value, B_TYPE_STRING);
            b_value_set_string (value, (gchar *)value_ptr);
            g_object_set_property(B_OBJECT(name_value->button),
              "label", value);
            btk_box_pack_start(BTK_BOX(name_value->column2),
              BTK_WIDGET(name_value->button), FALSE, FALSE, 10);
            break;
          }

        btk_box_pack_start (BTK_BOX (name_value->hbox),
                            BTK_WIDGET (name_value->column1),
                            TRUE, TRUE, 0);
        btk_box_pack_start (BTK_BOX (name_value->hbox),
                            BTK_WIDGET (name_value->column2),
                            TRUE, TRUE, 0);
        btk_container_add(BTK_CONTAINER(group->group_vbox),
          BTK_WIDGET(name_value->hbox));
        group->name_value = g_list_append (group->name_value, name_value);
      }
    else
      {
        btk_label_set_text(BTK_LABEL(name_value->label), label);
        switch (type)
          {
          case VALUE_STRING:
            btk_label_set_text(BTK_LABEL(name_value->string),
              (gchar *) value_ptr);
            break;
          case VALUE_BOOLEAN:
            btk_toggle_button_set_active(BTK_TOGGLE_BUTTON(name_value->boolean),
               *((gboolean *)value_ptr));
            btk_widget_set_sensitive(name_value->boolean, FALSE);
            break;
          case VALUE_TEXT:
            btk_entry_set_text (BTK_ENTRY (name_value->text),
              (gchar *) value_ptr);
            break;
          case VALUE_BUTTON:
            value = &(name_value->button_gval);
            memset (value, 0, sizeof (BValue));
            b_value_init (value, B_TYPE_STRING);
            b_value_set_string (value, (gchar *)value_ptr);
            g_object_set_property(B_OBJECT(name_value->button),
              "label", value);
            break;
          }
      }

    name_value->active = TRUE;
    name_value->type = type;
    name_value->signal_id = -1;

    btk_widget_show(BTK_WIDGET(name_value->label));

    switch (type)
      {
      case VALUE_STRING:
        btk_widget_show(BTK_WIDGET(name_value->string));
        break;
      case VALUE_BOOLEAN:
        btk_widget_show(BTK_WIDGET(name_value->boolean));
        break;
      case VALUE_TEXT:
        btk_widget_show(BTK_WIDGET(name_value->text));
        break;
      case VALUE_BUTTON:
        btk_widget_show(BTK_WIDGET(name_value->button));
        break;
      }

    btk_widget_show(BTK_WIDGET(name_value->column1));
    btk_widget_show(BTK_WIDGET(name_value->column2));
    btk_widget_show(BTK_WIDGET(name_value->hbox));
    btk_widget_show(BTK_WIDGET(group->group_vbox));

    return name_value;
}

static NameValue *
_print_key_value(TabNumber tab_n, gint group_number,
   const char *label, gpointer value, ValueType type)
{
  TabInfo *tab;
  GroupInfo *the_group;

  if (display_ascii)
    {
      if (type == VALUE_BOOLEAN)
        {
          if (*((gboolean *)value))
              g_print("\t%-30s\tTRUE\n", label);
          else
              g_print("\t%-30s\tFALSE\n", label);
        }
      else
        {
          g_print("\t%-30s\t%s\n", label, 
                  value ? (gchar *)value : "NULL");
        }
    }

  tab = nbook_tabs[tab_n];
  the_group = (GroupInfo *)g_list_nth_data(tab->groups, group_number);
  return _get_name_value(the_group, label, (gpointer)value, type);
}

static void
_finished_group(TabNumber tab_no, gint group_number)
{
    TabInfo *tab;
    GroupInfo *the_group;

    tab = nbook_tabs[tab_no];

    the_group = (GroupInfo *)g_list_nth_data(tab->groups, group_number);

    if (the_group->is_scrolled)
      btk_widget_show(BTK_WIDGET(the_group->scroll_outer_frame));

    btk_widget_show(BTK_WIDGET(the_group->frame));
    btk_widget_show(BTK_WIDGET(the_group->group_vbox));
    btk_widget_show(BTK_WIDGET(tab->main_box));
}

/* Signal handlers */

static gulong child_added_id = 0;
static gulong child_removed_id = 0;
static gulong state_change_id = 0;

static gulong text_caret_handler_id = 0;
static gulong text_inserted_id = 0;
static gulong text_deleted_id = 0;

static gulong table_row_inserted_id = 0;
static gulong table_column_inserted_id = 0;
static gulong table_row_deleted_id = 0;
static gulong table_column_deleted_id = 0;
static gulong table_row_reordered_id = 0;
static gulong table_column_reordered_id = 0;

static gulong property_id = 0;

static void
_update_handlers(BatkObject *obj)
{
    /* Remove signal handlers from object that had focus before */

    if (last_object != NULL && B_TYPE_CHECK_INSTANCE(last_object))
      {
        if (child_added_id != 0)
           g_signal_handler_disconnect(last_object, child_added_id);
        if (child_removed_id != 0)
           g_signal_handler_disconnect(last_object, child_removed_id);
        if (state_change_id != 0)
           g_signal_handler_disconnect(last_object, state_change_id);

        if (text_caret_handler_id != 0)
           g_signal_handler_disconnect(last_object, text_caret_handler_id);
        if (text_inserted_id != 0)
           g_signal_handler_disconnect(last_object, text_inserted_id);
        if (text_deleted_id != 0)
           g_signal_handler_disconnect(last_object, text_deleted_id);

        if (table_row_inserted_id != 0)
           g_signal_handler_disconnect(last_object, table_row_inserted_id);
        if (table_column_inserted_id != 0)
           g_signal_handler_disconnect(last_object, table_column_inserted_id);
        if (table_row_deleted_id != 0)
           g_signal_handler_disconnect(last_object, table_row_deleted_id);
        if (table_column_deleted_id != 0)
           g_signal_handler_disconnect(last_object, table_column_deleted_id);
        if (table_row_reordered_id != 0)
           g_signal_handler_disconnect(last_object, table_row_reordered_id);
        if (table_column_reordered_id != 0)
           g_signal_handler_disconnect(last_object, table_column_reordered_id);
        if (property_id != 0)
           g_signal_handler_disconnect(last_object, property_id);

        g_object_unref(last_object);
      }

    last_object = NULL;

    child_added_id = 0;
    child_removed_id = 0;
    text_caret_handler_id = 0;
    text_inserted_id = 0;
    text_deleted_id = 0;
    table_row_inserted_id = 0;
    table_column_inserted_id = 0;
    table_row_deleted_id = 0;
    table_column_deleted_id = 0;
    table_row_reordered_id = 0;
    table_column_reordered_id = 0;
    property_id = 0;

    if (!B_TYPE_CHECK_INSTANCE(obj))
        return;

    g_object_ref(obj);
    last_object = obj;

    /* Add signal handlers to object that now has focus. */

    if (BATK_IS_OBJECT(obj))
      {
         child_added_id = g_signal_connect_closure (obj,
                "children_changed::add",
                g_cclosure_new (G_CALLBACK (_notify_object_child_added),
                NULL, NULL), FALSE);

         child_removed_id = g_signal_connect_closure (obj,
                "children_changed::remove",
                g_cclosure_new (G_CALLBACK (_notify_object_child_removed),
                NULL, NULL), FALSE);

         state_change_id = g_signal_connect_closure (obj,
                "state_change",
                g_cclosure_new (G_CALLBACK (_notify_object_state_change),
                NULL, NULL), FALSE);
      }

    if (BATK_IS_TEXT(obj))
      {
        text_caret_handler_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("text_caret_moved", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_caret_handler),
                NULL, NULL), FALSE);
        text_inserted_id = g_signal_connect_closure (obj,
                "text_changed::insert",
                g_cclosure_new (G_CALLBACK (_notify_text_insert_handler),
                NULL, NULL), FALSE);
        text_deleted_id = g_signal_connect_closure (obj,
                "text_changed::delete",
                g_cclosure_new (G_CALLBACK (_notify_text_delete_handler),
                NULL, NULL), FALSE);
      }

    if (BATK_IS_TABLE(obj))
      {
        table_row_inserted_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("row_inserted", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_table_row_inserted),
                NULL, NULL), FALSE);
        table_column_inserted_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("column_inserted", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_table_column_inserted),
                NULL, NULL), FALSE);
        table_row_deleted_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("row_deleted", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_table_row_deleted),
                NULL, NULL), FALSE);
        table_column_deleted_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("column_deleted", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_table_column_deleted),
                NULL, NULL), FALSE);
        table_row_reordered_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("row_reordered", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_table_row_reordered),
                NULL, NULL), FALSE);
        table_column_reordered_id = g_signal_connect_closure_by_id (obj,
                g_signal_lookup ("column_reordered", B_OBJECT_TYPE (obj)),
                0, g_cclosure_new (G_CALLBACK (_notify_table_column_reordered),
                NULL, NULL), FALSE);
      }

    property_id = g_signal_connect_closure_by_id (obj,
      g_signal_lookup ("property_change", B_OBJECT_TYPE (obj)),
      0, g_cclosure_new (G_CALLBACK (_property_change_handler),
      NULL, NULL),
          FALSE);
}

/* Text signals */

static void
_notify_text_insert_handler (BObject *obj, int position, int offset)
{
    gchar *text = batk_text_get_text (BATK_TEXT (obj), position, position + offset);
    gchar *output_str = g_strdup_printf("position %d, length %d text: %s",
      position, offset,  text ? text: "<NULL>");
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TEXT,
      "Text Insert", output_str);
    g_free(output_str);
}

static void
_notify_text_delete_handler (BObject *obj, int position, int offset)
{
    gchar *text = batk_text_get_text (BATK_TEXT (obj), position, position + offset);
    gchar *output_str = g_strdup_printf("position %d, length %d text: %s",
      position, offset,  text ? text: "<NULL>");
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TEXT,
      "Text Delete", output_str);
    g_free(output_str);
}

static void
_notify_caret_handler (BObject *obj, int position)
{
    gchar *output_str = g_strdup_printf("position %d", position);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TEXT,
      "Text Caret Moved", output_str);
    g_free(output_str);
}

/* Table signals */

static void
_notify_table_row_inserted (BObject *obj, gint start_offset,
  gint length)
{
    gchar *output_str =
      g_strdup_printf("position %d, num of rows inserted %d!\n",
      start_offset, length);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TABLE,
      "Table Row Inserted", output_str);
    g_free(output_str);
}

static void
_notify_table_column_inserted (BObject *obj, gint start_offset,
  gint length)
{
    gchar *output_str =
      g_strdup_printf("position %d, num of rows inserted %d!\n",
      start_offset, length);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TABLE,
      "Table Column Inserted", output_str);
    g_free(output_str);
}

static void
_notify_table_row_deleted (BObject *obj, gint start_offset,
  gint length)
{
    gchar *output_str = g_strdup_printf("position %d, num of rows inserted %d!\n",
      start_offset, length);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TABLE,
      "Table Row Deleted", output_str);
    g_free(output_str);
}

static void
_notify_table_column_deleted (BObject *obj, gint start_offset,
  gint length)
{
    gchar *output_str = g_strdup_printf("position %d, num of rows inserted %d!\n",
      start_offset, length);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TABLE,
      "Table Column Deleted", output_str);
    g_free(output_str);
}

static void
_notify_table_row_reordered (BObject *obj)
{
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TABLE,
      "Table Row Reordered", NULL);
}

static void
_notify_table_column_reordered (BObject *obj)
{
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_TABLE,
      "Table Column Reordered", NULL);
}

/* Object signals */

static void
_notify_object_child_added (BObject *obj, gint index,
  BatkObject *child)
{
    gchar *output_str = g_strdup_printf("index %d", index);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_OBJECT,
      "Child Added", output_str);
    g_free(output_str);
}

static void
_notify_object_child_removed (BObject *obj, gint index,
  BatkObject *child)
{
    gchar *output_str = g_strdup_printf("index %d", index);
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_OBJECT,
      "Child Removed", output_str);
    g_free(output_str);
}

static void 
_notify_object_state_change (BObject *obj, gchar *name, gboolean set)
{
    gchar *output_str = g_strdup_printf("name %s %s set", 
                        name, set ? "is" : "was");
    _print_signal(BATK_OBJECT(obj), FERRET_SIGNAL_OBJECT,
      "State Change", output_str);
    g_free(output_str);
}


/* Function to print signals */

static void
_print_signal(BatkObject *aobject, FerretSignalType type,
  const char *name, const char *info)
{
    TabNumber top_tab = btk_notebook_get_current_page (notebook) + OBJECT;

    if (no_signals)
      return;

    if (display_ascii)
      {
        if (info != NULL)
            g_print("SIGNAL:\t%-34s\t%s\n", name, info);
        else
            g_print("SIGNAL:\t%-34s\n", name);
      }

    if (use_festival)
      {
        if ((type == FERRET_SIGNAL_TEXT) && (!strncmp(name, "Text Caret", 10)))
          {
            _speak_caret_event (aobject);
         }
        else if (type == FERRET_SIGNAL_TEXT)
          {
            last_caret_offset = batk_text_get_caret_offset (BATK_TEXT (aobject));
          }
      }

    if (use_magnifier && BATK_IS_TEXT (aobject) &&
        (type == FERRET_SIGNAL_TEXT) && (!strncmp(name, "Text Caret", 10)))
      {
        gint x, y, w, h;
        gint caret_offset = batk_text_get_caret_offset (BATK_TEXT (aobject));
        batk_text_get_character_extents ( BATK_TEXT (aobject), caret_offset, &x, &y, &w, &h, BATK_XY_SCREEN);
        _send_to_magnifier (x, y, w, h);
      }

    if ((type == FERRET_SIGNAL_TEXT && top_tab == TEXT) ||
        (type == FERRET_SIGNAL_TABLE && top_tab == TABLE) ||
        (type == FERRET_SIGNAL_OBJECT && top_tab == OBJECT))
      {
        if (display_ascii)
            g_print("Updating tab\n");

        _update(top_tab, aobject);
      }
}

/* Update functions */

static void
_update (TabNumber top_tab, BatkObject *aobject)
{
    gint group_num;

    if (top_tab >= OBJECT && top_tab < END_TABS)
    {
       _clear_tab(top_tab);
    }

    if (top_tab == OBJECT && BATK_IS_OBJECT(aobject))
      {
        group_num = _print_object(aobject);
        _finished_group(OBJECT, group_num);
        group_num = _print_relation(aobject);
        _finished_group(OBJECT, group_num);
        group_num = _print_state(aobject);
        _finished_group(OBJECT, group_num);
      }
    if (top_tab == TEXT && BATK_IS_TEXT(aobject))
      {
        group_num = _print_text(BATK_TEXT (aobject));
        _finished_group(TEXT, group_num);
        group_num = _print_text_attributes(BATK_TEXT (aobject));
        _finished_group(TEXT, group_num);
      }
    if (top_tab == SELECTION && BATK_IS_SELECTION(aobject))
      {
        group_num = _print_selection(BATK_SELECTION (aobject));
        _finished_group(SELECTION, group_num);
      }
    if (top_tab == TABLE && BATK_IS_TABLE(aobject))
      {
        group_num = _print_table(BATK_TABLE (aobject));
        _finished_group(TABLE, group_num);
      }
    if (top_tab == ACTION && BATK_IS_ACTION(aobject))
      {
        group_num = _print_action(BATK_ACTION (aobject));
        _finished_group(ACTION, group_num);
      }
    if (top_tab == COMPONENT && BATK_IS_COMPONENT(aobject))
      {
        group_num = _print_component(BATK_COMPONENT(aobject));
        _finished_group(COMPONENT, group_num);
      }
    if (top_tab == IMAGE && BATK_IS_IMAGE(aobject))
      {
        group_num = _print_image(BATK_IMAGE (aobject));
        _finished_group(IMAGE, group_num);
      }
    if (top_tab == VALUE && BATK_IS_VALUE(aobject))
      {
        group_num = _print_value(BATK_VALUE(aobject));
        _finished_group(VALUE, group_num);
      }
}

static void
_update_current_page(BtkNotebook *notebook, gpointer p, guint current_page)
{
  _update(current_page+OBJECT, last_object);
}

/* Property listeners */

static void _property_change_handler (BatkObject *obj,
  BatkPropertyValues *values)
{
    TabNumber top_tab = btk_notebook_get_current_page (notebook) + OBJECT;

    if (no_signals)
      return;

   /*
    * Only process if the property change corrisponds to the current
    * object
    */
    if (obj != last_object)
      {
        if (display_ascii)
          {
            g_print("\nProperty change event <%s> for object not in focus\n",
                values->property_name);
          }

        return;
      }

    if (display_ascii)
      {
        g_print("\nProperty change event <%s> occurred.\n",
          values->property_name);
      }

   /*
    * Update the top tab if a property changes.
    *
    * We may be able to ignore some property changes if they do not
    * change anything in ferret.
    */

    if (top_tab == OBJECT &&
       ((strcmp (values->property_name, "accessible-name") == 0) ||
        (strcmp (values->property_name, "accessible-description") == 0) ||
        (strcmp (values->property_name, "accessible-parent") == 0) ||
        (strcmp (values->property_name, "accessible-value") == 0) ||
        (strcmp (values->property_name, "accessible-role") == 0) ||
        (strcmp (values->property_name, "accessible-component-layout") == 0) ||
        (strcmp (values->property_name, "accessible-component-mdi-zorder") == 0) ||
        (strcmp (values->property_name, "accessible-table-caption") == 0) ||
        (strcmp (values->property_name,
                 "accessible-table-column-description") == 0) ||
        (strcmp (values->property_name,
                 "accessible-table-column-header") == 0) ||
        (strcmp (values->property_name,
                 "accessible-table-row-description") == 0) ||
        (strcmp (values->property_name,
                 "accessible-table-row-header") == 0) ||
        (strcmp (values->property_name, "accessible-table-summary") == 0)))
      {
        if (display_ascii)
            g_print("Updating tab\n");

        _update(top_tab, last_object);
      }
    else if (top_tab == VALUE &&
        (strcmp (values->property_name, "accessible-value") == 0))
      {
        if (display_ascii)
            g_print("Updating tab\n");

        _update(top_tab, last_object);
      }
}

/* Action button callback function */

void _action_cb(BtkWidget *widget, gpointer  *userdata)
{
   NameValue *nv = (NameValue *)userdata;
   batk_action_do_action(BATK_ACTION(nv->batkobj), nv->action_num);
}

/* Menu-bar callback functions */

void _toggle_terminal(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
       display_ascii = TRUE;
   else
       display_ascii = FALSE;
}

void _toggle_no_signals(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
       no_signals = TRUE;
   else
       no_signals = FALSE;
}

void _toggle_magnifier(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
       use_magnifier = TRUE;
   else
       use_magnifier = FALSE;
}

void _toggle_festival(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
       use_festival = TRUE;
   else
       use_festival = FALSE;
}

void _toggle_festival_terse(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
     {
        say_role = FALSE;
        say_accel = FALSE;
     }
   else
     {
        say_role = TRUE;
        say_accel = TRUE;
     }
}

void _toggle_trackmouse(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
     {
        mouse_watcher_focus_id =
          batk_add_global_event_listener(_mouse_watcher,
          "Btk:BtkWidget:enter_notify_event");
        mouse_watcher_button_id =
          batk_add_global_event_listener(_button_watcher,
          "Btk:BtkWidget:button_press_event");
       track_mouse = TRUE;
     }
   else
     {
       if (mouse_watcher_focus_id != -1)
         {
           batk_remove_global_event_listener(mouse_watcher_focus_id);
           batk_remove_global_event_listener(mouse_watcher_button_id);
           track_mouse = FALSE;
         }
     }
}

void _toggle_trackfocus(BtkCheckMenuItem *checkmenuitem,
  gpointer user_data)
{
   if (checkmenuitem->active)
     {
       track_focus = TRUE;
       focus_tracker_id = batk_add_focus_tracker (_print_accessible);
     }
   else
     { 
       g_print ("No longer tracking focus.\n");
       track_focus = FALSE;
       batk_remove_focus_tracker (focus_tracker_id);
     }
}
