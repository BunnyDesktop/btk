#include <btk/btk.h>

#ifdef BDK_WINDOWING_DIRECTFB
#define bdk_display bdk_display_directfb
#include <bdk/directfb/bdkdirectfb.h>
#undef bdk_display
#undef BDK_DISPLAY
#undef BDK_ROOT_WINDOW
#endif

#ifdef BDK_WINDOWING_QUARTZ
#if HAVE_OBJC
#define bdk_display bdk_display_quartz
#include <bdk/quartz/bdkquartz.h>
#undef bdk_display
#undef BDK_DISPLAY
#undef BDK_ROOT_WINDOW
#endif
#endif

#ifndef __OBJC__
#ifdef BDK_WINDOWING_WIN32
#define bdk_display bdk_display_win32
#include <bdk/win32/bdkwin32.h>
#undef bdk_display
#undef BDK_DISPLAY
#undef BDK_ROOT_WINDOW
#endif
#endif

#ifdef BDK_WINDOWING_X11
#define bdk_display bdk_display_x11
#include <bdk/x11/bdkx.h>
#undef bdk_display
#undef BDK_DISPLAY
#undef BDK_ROOT_WINDOW
#endif

int main() { return 0; }
