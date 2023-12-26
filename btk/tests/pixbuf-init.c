#include "config.h"
#include <bunnylib.h>
#include <bunnylib/gstdio.h>

static gboolean
file_exists (const char *filename)
{
  GStatBuf statbuf;

  return g_stat (filename, &statbuf) == 0;
}

void
pixbuf_init (void)
{
  if (file_exists ("../../bdk-pixbuf/libpixbufloader-pnm.la"))
    g_setenv ("BDK_PIXBUF_MODULE_FILE", "../../bdk-pixbuf/bdk-pixbuf.loaders", TRUE);
}
