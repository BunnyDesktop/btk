#include "bdkalias.h"
/* This file is generated by bunnylib-genmarshal, do not modify it. This code is licensed under the same license as the containing project. Note that it links to GLib, so must comply with the LGPL linking clauses. */
#include <bunnylib-object.h>

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  b_value_get_boolean (v)
#define g_marshal_value_peek_char(v)     b_value_get_schar (v)
#define g_marshal_value_peek_uchar(v)    b_value_get_uchar (v)
#define g_marshal_value_peek_int(v)      b_value_get_int (v)
#define g_marshal_value_peek_uint(v)     b_value_get_uint (v)
#define g_marshal_value_peek_long(v)     b_value_get_long (v)
#define g_marshal_value_peek_ulong(v)    b_value_get_ulong (v)
#define g_marshal_value_peek_int64(v)    b_value_get_int64 (v)
#define g_marshal_value_peek_uint64(v)   b_value_get_uint64 (v)
#define g_marshal_value_peek_enum(v)     b_value_get_enum (v)
#define g_marshal_value_peek_flags(v)    b_value_get_flags (v)
#define g_marshal_value_peek_float(v)    b_value_get_float (v)
#define g_marshal_value_peek_double(v)   b_value_get_double (v)
#define g_marshal_value_peek_string(v)   (char*) b_value_get_string (v)
#define g_marshal_value_peek_param(v)    b_value_get_param (v)
#define g_marshal_value_peek_boxed(v)    b_value_get_boxed (v)
#define g_marshal_value_peek_pointer(v)  b_value_get_pointer (v)
#define g_marshal_value_peek_object(v)   b_value_get_object (v)
#define g_marshal_value_peek_variant(v)  b_value_get_variant (v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses BValues directly, which is UNSUPPORTED API.
 *          Do not access BValues directly in your code. Instead, use the
 *          b_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_variant(v)  (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */

/* VOID:POINTER,POINTER,POINTER (./bdkmarshalers.list:3) */
void
_bdk_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
                                            BValue       *return_value B_GNUC_UNUSED,
                                            buint         n_param_values,
                                            const BValue *param_values,
                                            bpointer      invocation_hint B_GNUC_UNUSED,
                                            bpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_POINTER_POINTER) (bpointer data1,
                                                              bpointer arg1,
                                                              bpointer arg2,
                                                              bpointer arg3,
                                                              bpointer data2);
  GCClosure *cc = (GCClosure *) closure;
  bpointer data1, data2;
  GMarshalFunc_VOID__POINTER_POINTER_POINTER callback;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = b_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = b_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_pointer (param_values + 1),
            g_marshal_value_peek_pointer (param_values + 2),
            g_marshal_value_peek_pointer (param_values + 3),
            data2);
}

/* OBJECT:VOID (./bdkmarshalers.list:4) */
void
_bdk_marshal_OBJECT__VOID (GClosure     *closure,
                           BValue       *return_value,
                           buint         n_param_values,
                           const BValue *param_values,
                           bpointer      invocation_hint B_GNUC_UNUSED,
                           bpointer      marshal_data)
{
  typedef BObject* (*GMarshalFunc_OBJECT__VOID) (bpointer data1,
                                                 bpointer data2);
  GCClosure *cc = (GCClosure *) closure;
  bpointer data1, data2;
  GMarshalFunc_OBJECT__VOID callback;
  BObject* v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = b_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = b_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_OBJECT__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);

  b_value_take_object (return_value, v_return);
}

/* OBJECT:DOUBLE,DOUBLE (./bdkmarshalers.list:5) */
void
_bdk_marshal_OBJECT__DOUBLE_DOUBLE (GClosure     *closure,
                                    BValue       *return_value,
                                    buint         n_param_values,
                                    const BValue *param_values,
                                    bpointer      invocation_hint B_GNUC_UNUSED,
                                    bpointer      marshal_data)
{
  typedef BObject* (*GMarshalFunc_OBJECT__DOUBLE_DOUBLE) (bpointer data1,
                                                          bdouble arg1,
                                                          bdouble arg2,
                                                          bpointer data2);
  GCClosure *cc = (GCClosure *) closure;
  bpointer data1, data2;
  GMarshalFunc_OBJECT__DOUBLE_DOUBLE callback;
  BObject* v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = b_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = b_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_OBJECT__DOUBLE_DOUBLE) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_marshal_value_peek_double (param_values + 1),
                       g_marshal_value_peek_double (param_values + 2),
                       data2);

  b_value_take_object (return_value, v_return);
}

/* VOID:DOUBLE,DOUBLE,POINTER,POINTER (./bdkmarshalers.list:6) */
void
_bdk_marshal_VOID__DOUBLE_DOUBLE_POINTER_POINTER (GClosure     *closure,
                                                  BValue       *return_value B_GNUC_UNUSED,
                                                  buint         n_param_values,
                                                  const BValue *param_values,
                                                  bpointer      invocation_hint B_GNUC_UNUSED,
                                                  bpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__DOUBLE_DOUBLE_POINTER_POINTER) (bpointer data1,
                                                                    bdouble arg1,
                                                                    bdouble arg2,
                                                                    bpointer arg3,
                                                                    bpointer arg4,
                                                                    bpointer data2);
  GCClosure *cc = (GCClosure *) closure;
  bpointer data1, data2;
  GMarshalFunc_VOID__DOUBLE_DOUBLE_POINTER_POINTER callback;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = b_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = b_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__DOUBLE_DOUBLE_POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_double (param_values + 1),
            g_marshal_value_peek_double (param_values + 2),
            g_marshal_value_peek_pointer (param_values + 3),
            g_marshal_value_peek_pointer (param_values + 4),
            data2);
}

