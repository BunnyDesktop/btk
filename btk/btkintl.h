#ifndef __BTKINTL_H__
#define __BTKINTL_H__

#include <bunnylib/gi18n-lib.h>

#ifdef ENABLE_NLS
#define P_(String) g_dgettext(GETTEXT_PACKAGE "-properties",String)
#else 
#define P_(String) (String)
#endif

/* not really I18N-related, but also a string marker macro */
#define I_(string) g_intern_static_string (string)

#endif
