#ifndef BTK_DISABLE_DEPRECATED
/* This file is generated by bunnylib-genmarshal, do not modify it. This code is licensed under the same license as the containing project. Note that it links to GLib, so must comply with the LGPL linking clauses. */
#ifndef __BTK_MARSHAL_MARSHAL_H__
#define __BTK_MARSHAL_MARSHAL_H__

#include <bunnylib-object.h>

G_BEGIN_DECLS

/* BOOL:NONE (./btkmarshal.list:1) */
extern
void btk_marshal_BOOLEAN__VOID (GClosure     *closure,
                                GValue       *return_value,
                                guint         n_param_values,
                                const GValue *param_values,
                                gpointer      invocation_hint,
                                gpointer      marshal_data);

#define btk_marshal_BOOL__NONE	btk_marshal_BOOLEAN__VOID

/* BOOL:POINTER (./btkmarshal.list:2) */
extern
void btk_marshal_BOOLEAN__POINTER (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data);

#define btk_marshal_BOOL__POINTER	btk_marshal_BOOLEAN__POINTER

/* BOOL:POINTER,POINTER,INT,INT (./btkmarshal.list:3) */
extern
void btk_marshal_BOOLEAN__POINTER_POINTER_INT_INT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

#define btk_marshal_BOOL__POINTER_POINTER_INT_INT	btk_marshal_BOOLEAN__POINTER_POINTER_INT_INT

/* BOOL:POINTER,INT,INT (./btkmarshal.list:4) */
extern
void btk_marshal_BOOLEAN__POINTER_INT_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

#define btk_marshal_BOOL__POINTER_INT_INT	btk_marshal_BOOLEAN__POINTER_INT_INT

/* BOOL:POINTER,INT,INT,UINT (./btkmarshal.list:5) */
extern
void btk_marshal_BOOLEAN__POINTER_INT_INT_UINT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

#define btk_marshal_BOOL__POINTER_INT_INT_UINT	btk_marshal_BOOLEAN__POINTER_INT_INT_UINT

/* BOOL:POINTER,STRING,STRING,POINTER (./btkmarshal.list:6) */
extern
void btk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

#define btk_marshal_BOOL__POINTER_STRING_STRING_POINTER	btk_marshal_BOOLEAN__POINTER_STRING_STRING_POINTER

/* ENUM:ENUM (./btkmarshal.list:7) */
extern
void btk_marshal_ENUM__ENUM (GClosure     *closure,
                             GValue       *return_value,
                             guint         n_param_values,
                             const GValue *param_values,
                             gpointer      invocation_hint,
                             gpointer      marshal_data);

/* INT:POINTER (./btkmarshal.list:8) */
extern
void btk_marshal_INT__POINTER (GClosure     *closure,
                               GValue       *return_value,
                               guint         n_param_values,
                               const GValue *param_values,
                               gpointer      invocation_hint,
                               gpointer      marshal_data);

/* INT:POINTER,CHAR,CHAR (./btkmarshal.list:9) */
extern
void btk_marshal_INT__POINTER_CHAR_CHAR (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* NONE:BOOL (./btkmarshal.list:10) */
#define btk_marshal_VOID__BOOLEAN	g_cclosure_marshal_VOID__BOOLEAN

#define btk_marshal_NONE__BOOL	btk_marshal_VOID__BOOLEAN

/* NONE:BOXED (./btkmarshal.list:11) */
#define btk_marshal_VOID__BOXED	g_cclosure_marshal_VOID__BOXED

#define btk_marshal_NONE__BOXED	btk_marshal_VOID__BOXED

/* NONE:ENUM (./btkmarshal.list:12) */
#define btk_marshal_VOID__ENUM	g_cclosure_marshal_VOID__ENUM

#define btk_marshal_NONE__ENUM	btk_marshal_VOID__ENUM

/* NONE:ENUM,FLOAT (./btkmarshal.list:13) */
extern
void btk_marshal_VOID__ENUM_FLOAT (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data);

#define btk_marshal_NONE__ENUM_FLOAT	btk_marshal_VOID__ENUM_FLOAT

/* NONE:ENUM,FLOAT,BOOL (./btkmarshal.list:14) */
extern
void btk_marshal_VOID__ENUM_FLOAT_BOOLEAN (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

#define btk_marshal_NONE__ENUM_FLOAT_BOOL	btk_marshal_VOID__ENUM_FLOAT_BOOLEAN

/* NONE:INT (./btkmarshal.list:15) */
#define btk_marshal_VOID__INT	g_cclosure_marshal_VOID__INT

#define btk_marshal_NONE__INT	btk_marshal_VOID__INT

/* NONE:INT,INT (./btkmarshal.list:16) */
extern
void btk_marshal_VOID__INT_INT (GClosure     *closure,
                                GValue       *return_value,
                                guint         n_param_values,
                                const GValue *param_values,
                                gpointer      invocation_hint,
                                gpointer      marshal_data);

#define btk_marshal_NONE__INT_INT	btk_marshal_VOID__INT_INT

/* NONE:INT,INT,POINTER (./btkmarshal.list:17) */
extern
void btk_marshal_VOID__INT_INT_POINTER (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

#define btk_marshal_NONE__INT_INT_POINTER	btk_marshal_VOID__INT_INT_POINTER

/* NONE:NONE (./btkmarshal.list:18) */
#define btk_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

#define btk_marshal_NONE__NONE	btk_marshal_VOID__VOID

/* NONE:OBJECT (./btkmarshal.list:19) */
#define btk_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT

#define btk_marshal_NONE__OBJECT	btk_marshal_VOID__OBJECT

/* NONE:POINTER (./btkmarshal.list:20) */
#define btk_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

#define btk_marshal_NONE__POINTER	btk_marshal_VOID__POINTER

/* NONE:POINTER,INT (./btkmarshal.list:21) */
extern
void btk_marshal_VOID__POINTER_INT (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_INT	btk_marshal_VOID__POINTER_INT

/* NONE:POINTER,POINTER (./btkmarshal.list:22) */
extern
void btk_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_POINTER	btk_marshal_VOID__POINTER_POINTER

/* NONE:POINTER,POINTER,POINTER (./btkmarshal.list:23) */
extern
void btk_marshal_VOID__POINTER_POINTER_POINTER (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_POINTER_POINTER	btk_marshal_VOID__POINTER_POINTER_POINTER

/* NONE:POINTER,STRING,STRING (./btkmarshal.list:24) */
extern
void btk_marshal_VOID__POINTER_STRING_STRING (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_STRING_STRING	btk_marshal_VOID__POINTER_STRING_STRING

/* NONE:POINTER,UINT (./btkmarshal.list:25) */
extern
void btk_marshal_VOID__POINTER_UINT (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_UINT	btk_marshal_VOID__POINTER_UINT

/* NONE:POINTER,UINT,ENUM (./btkmarshal.list:26) */
extern
void btk_marshal_VOID__POINTER_UINT_ENUM (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_UINT_ENUM	btk_marshal_VOID__POINTER_UINT_ENUM

/* NONE:POINTER,POINTER,UINT,UINT (./btkmarshal.list:27) */
extern
void btk_marshal_VOID__POINTER_POINTER_UINT_UINT (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_POINTER_UINT_UINT	btk_marshal_VOID__POINTER_POINTER_UINT_UINT

/* NONE:POINTER,INT,INT,POINTER,UINT,UINT (./btkmarshal.list:28) */
extern
void btk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT (GClosure     *closure,
                                                          GValue       *return_value,
                                                          guint         n_param_values,
                                                          const GValue *param_values,
                                                          gpointer      invocation_hint,
                                                          gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_INT_INT_POINTER_UINT_UINT	btk_marshal_VOID__POINTER_INT_INT_POINTER_UINT_UINT

/* NONE:POINTER,UINT,UINT (./btkmarshal.list:29) */
extern
void btk_marshal_VOID__POINTER_UINT_UINT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

#define btk_marshal_NONE__POINTER_UINT_UINT	btk_marshal_VOID__POINTER_UINT_UINT

/* NONE:STRING (./btkmarshal.list:31) */
#define btk_marshal_VOID__STRING	g_cclosure_marshal_VOID__STRING

#define btk_marshal_NONE__STRING	btk_marshal_VOID__STRING

/* NONE:STRING,INT,POINTER (./btkmarshal.list:32) */
extern
void btk_marshal_VOID__STRING_INT_POINTER (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

#define btk_marshal_NONE__STRING_INT_POINTER	btk_marshal_VOID__STRING_INT_POINTER

/* NONE:UINT (./btkmarshal.list:33) */
#define btk_marshal_VOID__UINT	g_cclosure_marshal_VOID__UINT

#define btk_marshal_NONE__UINT	btk_marshal_VOID__UINT

/* NONE:UINT,POINTER,UINT,ENUM,ENUM,POINTER (./btkmarshal.list:34) */
extern
void btk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER (GClosure     *closure,
                                                            GValue       *return_value,
                                                            guint         n_param_values,
                                                            const GValue *param_values,
                                                            gpointer      invocation_hint,
                                                            gpointer      marshal_data);

#define btk_marshal_NONE__UINT_POINTER_UINT_ENUM_ENUM_POINTER	btk_marshal_VOID__UINT_POINTER_UINT_ENUM_ENUM_POINTER

/* NONE:UINT,POINTER,UINT,UINT,ENUM (./btkmarshal.list:35) */
extern
void btk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

#define btk_marshal_NONE__UINT_POINTER_UINT_UINT_ENUM	btk_marshal_VOID__UINT_POINTER_UINT_UINT_ENUM

/* NONE:UINT,STRING (./btkmarshal.list:36) */
extern
void btk_marshal_VOID__UINT_STRING (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data);

#define btk_marshal_NONE__UINT_STRING	btk_marshal_VOID__UINT_STRING


G_END_DECLS

#endif /* __BTK_MARSHAL_MARSHAL_H__ */
#endif /* BTK_DISABLE_DEPRECATED */
