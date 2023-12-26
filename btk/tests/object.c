/* Btk+ object tests
 * Copyright (C) 2007 Imendio AB
 * Authors: Tim Janik
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
#include <btk/btk.h>
#include <string.h>

/* --- helper macros for property value generation --- */
/* dvalue=+0: generate minimum value
 * dvalue=.x: generate value within value range proportional to x.
 * dvalue=+1: generate maximum value
 * dvalue=-1: generate random value within value range
 * dvalue=+2: initialize value from default_value
 */
#define ASSIGN_VALUE(__b_value_set_func, __value, PSPECTYPE, __pspec, __default_value, __minimum, __maximum, __dvalue) do { \
  PSPECTYPE __p = (PSPECTYPE) __pspec; \
  __b_value_set_func (__value, SELECT_VALUE (__dvalue, __p->__default_value, __p->__minimum, __p->__maximum)); \
} while (0)
#define SELECT_VALUE(__dvalue, __default_value, __minimum, __maximum) ( \
  __dvalue >= 0 && __dvalue <= 1 ? __minimum * (1 - __dvalue) + __dvalue * __maximum : \
    __dvalue <= -1 ? g_test_rand_double_range (__minimum, __maximum) : \
      __default_value)
#define SELECT_NAME(__dvalue) ( \
  __dvalue == 0 ? "minimum" : \
    __dvalue == 1 ? "maximum" : \
      __dvalue >= +2 ? "default" : \
        __dvalue == 0.5 ? "medium" : \
          __dvalue > 0 && __dvalue < 1 ? "fractional" : \
            "random")
#define MATCH_ANY_VALUE         ((void*) 0xf1874c23)

/* --- property blacklists --- */
typedef struct {
  const char   *type_name;
  const char   *name;
  gconstpointer value;
} IgnoreProperty;
static const IgnoreProperty*
list_ignore_properties (gboolean buglist)
{
  /* currently untestable properties */
  static const IgnoreProperty ignore_properties[] = {
    { "BtkCurve",               "",                     NULL, },                        /* Just ignore it, not worth fixing */
    { "BtkContainer",           "child",                NULL, },                        /* needs working child widget */
    { "BtkRadioMenuItem",       "group",                NULL, },                        /* needs working sibling */
    { "BtkWidget",              "parent",               NULL, },                        /* needs working parent widget */
    { "BtkCList",               "selection-mode",       (void*) BTK_SELECTION_NONE, },
    { "BtkWidget",              "has-default",          (void*) TRUE, },                /* conflicts with toplevel-less widgets */
    { "BtkWidget",              "screen",               NULL, },
    { "BtkWindow",              "type-hint",            (void*) BDK_WINDOW_TYPE_HINT_DND, }, /* conflicts with ::visible=TRUE */
    { "BtkCellView",            "background",           (void*) "", },                  /* "" is not a valid background color */
    { "BtkColorButton",         "color",                (void*) NULL, },                /* not a valid boxed color */
    { "BtkInputDialog",         "has-separator",        (void*) MATCH_ANY_VALUE, },     /* property disabled */
    { "BtkMessageDialog",       "has-separator",        (void*) MATCH_ANY_VALUE, },     /* property disabled */
    { "BtkFontSelectionDialog", "has-separator",        (void*) MATCH_ANY_VALUE, },     /* property disabled */
    { "BtkColorSelectionDialog","has-separator",        (void*) MATCH_ANY_VALUE, },     /* property disabled */
    { "BtkColorSelection",      "child",                NULL, },
    { "BtkColorSelection",      "current-color",        (void*) NULL, },                /* not a valid boxed color */
    { "BtkComboBox",            "row-span-column",      (void*) MATCH_ANY_VALUE },      /* BtkComboBoxEntry needs a tree model for this */
    { "BtkComboBox",            "column-span-column",   (void*) MATCH_ANY_VALUE },      /* BtkComboBoxEntry needs a tree model for this */
    { "BtkFileChooserButton",   "select-multiple",      (void*) MATCH_ANY_VALUE },      /* property disabled */
    { "BtkFileChooserButton",   "action",               (void*) BTK_FILE_CHOOSER_ACTION_SAVE },
    { "BtkFileChooserButton",   "action",               (void*) BTK_FILE_CHOOSER_ACTION_CREATE_FOLDER },
    { "BtkFileChooserWidget",   "select-multiple",      (void*) 0x1 },                  /* property conflicts */
    { "BtkFileChooserDialog",   "select-multiple",      (void*) MATCH_ANY_VALUE },      /* property disabled */
    { "BtkMenu",                "accel-path",           (void*) MATCH_ANY_VALUE },      /* has odd restrictions in the setter */
    { "BtkMenuItem",            "accel-path",           (void*) MATCH_ANY_VALUE },      /* has odd restrictions in the setter */
    { "BtkRecentChooserMenu",   "select-multiple",      (void*) MATCH_ANY_VALUE },      /* property disabled */
    { "BtkTextView",            "overwrite",            (void*) MATCH_ANY_VALUE },      /* needs text buffer */
    { "BtkToolbar",             "icon-size",            (void*) BTK_ICON_SIZE_INVALID },
    { NULL, NULL, NULL }
  };
  /* properties suspected to be Bdk/Btk+ bugs */
  static const IgnoreProperty bug_properties[] = {
    { "BtkComboBox",            "active",               (void*) MATCH_ANY_VALUE },      /* FIXME: triggers NULL model bug */
    { "BtkCTree",               "spacing",              (void*) MATCH_ANY_VALUE },      /* FIXME: triggers signedness bug */
    { "BtkFileChooserButton",   "local-only",           (void*) MATCH_ANY_VALUE },      /* FIXME: triggers NULL path assertion */
    { "BtkFileChooserDialog",   "local-only",           (void*) MATCH_ANY_VALUE },      /* FIXME: triggers NULL path assertion */
    { "BtkFileChooserWidget",   "local-only",           (void*) MATCH_ANY_VALUE },      /* FIXME: triggers NULL path assertion */
    { "BtkMenu",                "tearoff-state",        (void*) MATCH_ANY_VALUE },      /* FIXME: triggers NULL widget cast */
    { "BtkText",                "text-position",        (void*) MATCH_ANY_VALUE },      /* FIXME: segfaults, fix property minimum/maximum */
    { NULL, NULL, NULL }
  };
  if (buglist)
    return bug_properties;
  else
    return ignore_properties;
}

/* --- test functions --- */
static void
pspec_select_value (BParamSpec *pspec,
                    BValue     *value,
                    double      dvalue)
{
  /* generate a value suitable for pspec */
  if (G_IS_PARAM_SPEC_CHAR (pspec))
    ASSIGN_VALUE (b_value_set_char, value, BParamSpecChar*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_UCHAR (pspec))
    ASSIGN_VALUE (b_value_set_uchar, value, BParamSpecUChar*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_INT (pspec))
    ASSIGN_VALUE (b_value_set_int, value, BParamSpecInt*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_UINT (pspec))
    ASSIGN_VALUE (b_value_set_uint, value, BParamSpecUInt*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_LONG (pspec))
    ASSIGN_VALUE (b_value_set_long, value, BParamSpecLong*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_ULONG (pspec))
    ASSIGN_VALUE (b_value_set_ulong, value, BParamSpecULong*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_INT64 (pspec))
    ASSIGN_VALUE (b_value_set_int64, value, BParamSpecInt64*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_UINT64 (pspec))
    ASSIGN_VALUE (b_value_set_uint64, value, BParamSpecUInt64*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_FLOAT (pspec))
    ASSIGN_VALUE (b_value_set_float, value, BParamSpecFloat*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_DOUBLE (pspec))
    ASSIGN_VALUE (b_value_set_double, value, BParamSpecDouble*, pspec, default_value, minimum, maximum, dvalue);
  else if (G_IS_PARAM_SPEC_BOOLEAN (pspec))
    b_value_set_boolean (value, SELECT_VALUE (dvalue, ((BParamSpecBoolean*) pspec)->default_value, FALSE, TRUE));
  else if (G_IS_PARAM_SPEC_UNICHAR (pspec))
    b_value_set_uint (value, SELECT_VALUE (dvalue, ((BParamSpecUnichar*) pspec)->default_value, FALSE, TRUE));
  else if (G_IS_PARAM_SPEC_GTYPE (pspec))
    b_value_set_gtype (value, SELECT_VALUE ((int) dvalue, ((BParamSpecGType*) pspec)->is_a_type, 0, BTK_TYPE_WIDGET));
  else if (G_IS_PARAM_SPEC_STRING (pspec))
    {
      BParamSpecString *sspec = (BParamSpecString*) pspec;
      if (dvalue >= +2)
        b_value_set_string (value, sspec->default_value);
      if (dvalue > 0 && sspec->cset_first && sspec->cset_nth)
        b_value_take_string (value, g_strdup_printf ("%c%c", sspec->cset_first[0], sspec->cset_nth[0]));
      else /* if (sspec->ensure_non_null) */
        b_value_set_string (value, "");
    }
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      BParamSpecEnum *espec = (BParamSpecEnum*) pspec;
      if (dvalue >= +2)
        b_value_set_enum (value, espec->default_value);
      if (dvalue >= 0 && dvalue <= 1)
        b_value_set_enum (value, espec->enum_class->values[(int) ((espec->enum_class->n_values - 1) * dvalue)].value);
      else if (dvalue <= -1)
        b_value_set_enum (value, espec->enum_class->values[g_test_rand_int_range (0, espec->enum_class->n_values)].value);
    }
  else if (G_IS_PARAM_SPEC_FLAGS (pspec))
    {
      BParamSpecFlags *fspec = (BParamSpecFlags*) pspec;
      if (dvalue >= +2)
        b_value_set_flags (value, fspec->default_value);
      if (dvalue >= 0 && dvalue <= 1)
        b_value_set_flags (value, fspec->flags_class->values[(int) ((fspec->flags_class->n_values - 1) * dvalue)].value);
      else if (dvalue <= -1)
        b_value_set_flags (value, fspec->flags_class->values[g_test_rand_int_range (0, fspec->flags_class->n_values)].value);
    }
  /* unimplemented:
   * G_IS_PARAM_SPEC_PARAM
   * G_IS_PARAM_SPEC_BOXED
   * G_IS_PARAM_SPEC_POINTER
   * G_IS_PARAM_SPEC_VALUE_ARRAY
   * G_IS_PARAM_SPEC_OBJECT
   */
}

static gpointer
value_as_pointer (BValue *value)
{
  if (b_value_fits_pointer (value))
    return b_value_peek_pointer (value);
  if (G_VALUE_HOLDS_BOOLEAN (value))
    return GINT_TO_POINTER(b_value_get_boolean (value));
  if (G_VALUE_HOLDS_CHAR (value))
    return (void*) (gssize) b_value_get_char (value);
  if (G_VALUE_HOLDS_UCHAR (value))
    return (void*) (gsize) b_value_get_uchar (value);
  if (G_VALUE_HOLDS_INT (value))
    return GINT_TO_POINTER(b_value_get_int (value));
  if (G_VALUE_HOLDS_UINT (value))
    return GUINT_TO_POINTER(b_value_get_uint (value));
  if (G_VALUE_HOLDS_LONG (value))
    return (void*) b_value_get_long (value);
  if (G_VALUE_HOLDS_ULONG (value))
    return (void*) b_value_get_ulong (value);
  if (G_VALUE_HOLDS_FLOAT (value))
    return (void*) (gssize) b_value_get_float (value);
  if (G_VALUE_HOLDS_DOUBLE (value))
    return (void*) (gssize) b_value_get_double (value);
  if (G_VALUE_HOLDS_ENUM (value))
    return (void*) (gssize) b_value_get_enum (value);
  if (G_VALUE_HOLDS_FLAGS (value))
    return (void*) (gsize) b_value_get_flags (value);
  return (void*) 0x1373babe;
}

static void
object_test_property (BObject           *object,
                      BParamSpec        *pspec,
                      double             dvalue)
{
  /* test setting of a normal writable property */
  if (pspec->flags & G_PARAM_WRITABLE &&
      !(pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)))
    {
      BValue value = { 0, };
      guint i;
      const IgnoreProperty *ignore_properties;
      /* select value to set */
      b_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      pspec_select_value (pspec, &value, dvalue);
      /* ignore untestable properties */
      ignore_properties = list_ignore_properties (FALSE);
      for (i = 0; ignore_properties[i].name; i++)
        if (g_strcmp0 ("", ignore_properties[i].name) ||
            (g_type_is_a (B_OBJECT_TYPE (object), g_type_from_name (ignore_properties[i].type_name)) &&
             strcmp (pspec->name, ignore_properties[i].name) == 0 &&
             (MATCH_ANY_VALUE == ignore_properties[i].value ||
              value_as_pointer (&value) == ignore_properties[i].value ||
              (G_VALUE_HOLDS_STRING (&value) &&
               strcmp (b_value_get_string (&value), ignore_properties[i].value) == 0))))
          break;
      /* ignore known property bugs if not testing thoroughly */
      if (ignore_properties[i].name == NULL && !g_test_thorough ())
        {
          ignore_properties = list_ignore_properties (TRUE);
          for (i = 0; ignore_properties[i].name; i++)
            if (g_type_is_a (B_OBJECT_TYPE (object), g_type_from_name (ignore_properties[i].type_name)) &&
                strcmp (pspec->name, ignore_properties[i].name) == 0 &&
                (MATCH_ANY_VALUE == ignore_properties[i].value ||
                 value_as_pointer (&value) == ignore_properties[i].value ||
                 (G_VALUE_HOLDS_STRING (&value) &&
                  strcmp (b_value_get_string (&value), ignore_properties[i].value) == 0)))
              break;
        }
      /* assign unignored properties */
      if (ignore_properties[i].name == NULL)
        {
          if (g_test_verbose ())
            g_print ("PropertyTest: %s::%s := (%s value (%s): %p)\n",
                     g_type_name (B_OBJECT_TYPE (object)), pspec->name,
                     SELECT_NAME (dvalue), g_type_name (G_VALUE_TYPE (&value)),
                     value_as_pointer (&value));
          g_object_set_property (object, pspec->name, &value);
        }
      /* cleanups */
      b_value_unset (&value);
    }
}

static void
widget_test_properties (BtkWidget   *widget,
                        double       dvalue)
{
  /* try setting all possible properties, according to dvalue */
  guint i, n_pspecs = 0;
  BParamSpec **pspecs = g_object_class_list_properties (B_OBJECT_GET_CLASS (widget), &n_pspecs);
  for (i = 0; i < n_pspecs; i++)
    {
      BParamSpec *pspec = pspecs[i];
      if (pspec->flags & G_PARAM_WRITABLE &&
          !(pspec->flags & (G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY)))
        object_test_property (B_OBJECT (widget), pspecs[i], dvalue);
    }
  g_free (pspecs);
}

static void
widget_property_tests (gconstpointer test_data)
{
  GType wtype = (GType) test_data;
  /* create widget */
  BtkWidget *widget = btk_widget_new (wtype, NULL);
  g_object_ref_sink (widget);
  /* test property values */
  widget_test_properties (widget,  +2); /* test default_value */
  widget_test_properties (widget,   0); /* test minimum */
  widget_test_properties (widget, 0.5); /* test medium */
  widget_test_properties (widget,   1); /* test maximum */
  widget_test_properties (widget,  -1); /* test random value */
  /* cleanup */
  btk_widget_destroy (widget);
  g_object_unref (widget);
}

extern void pixbuf_init (void);

/* --- main test program --- */
int
main (int   argc,
      char *argv[])
{
  const GType *otypes;
  guint i;
  /* initialize test program */
  pixbuf_init ();
  btk_test_init (&argc, &argv);
  btk_test_register_all_types ();
  /* install a property test for each widget type */
  otypes = btk_test_list_all_types (NULL);
  for (i = 0; otypes[i]; i++)
    if (g_type_is_a (otypes[i], BTK_TYPE_WIDGET) &&
        B_TYPE_IS_OBJECT (otypes[i]) &&
        !B_TYPE_IS_ABSTRACT (otypes[i]))
      {
        gchar *testpath = g_strdup_printf ("/properties/%s", g_type_name (otypes[i]));
        g_test_add_data_func (testpath, (void*) otypes[i], widget_property_tests);
        g_free (testpath);
      }
  return g_test_run ();
}
