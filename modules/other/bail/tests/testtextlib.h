#include <stdio.h>
#include <btk/btk.h>
#include <batk/batk.h>
#include "testlib.h"

void add_handlers (BatkObject *obj);
gint setup_gui (BatkObject *obj, TLruntest test);
void runtest(BatkObject *obj, gint win_num);
void _run_offset_test(BatkObject *obj, char * type, gint param_int1, BatkTextBoundary boundary);
void _notify_caret_handler (GObject *obj, int position);
void _notify_text_insert_handler (GObject *obj,
  int start_offset, int end_offset);
void _notify_text_delete_handler (GObject *obj,
  int start_offset, int end_offset);                                 

