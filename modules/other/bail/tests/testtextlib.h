#include <stdio.h>
#include <btk/btk.h>
#include <batk/batk.h>
#include "testlib.h"

void add_handlers (BatkObject *obj);
bint setup_gui (BatkObject *obj, TLruntest test);
void runtest(BatkObject *obj, bint win_num);
void _run_offset_test(BatkObject *obj, char * type, bint param_int1, BatkTextBoundary boundary);
void _notify_caret_handler (BObject *obj, int position);
void _notify_text_insert_handler (BObject *obj,
  int start_offset, int end_offset);
void _notify_text_delete_handler (BObject *obj,
  int start_offset, int end_offset);                                 

